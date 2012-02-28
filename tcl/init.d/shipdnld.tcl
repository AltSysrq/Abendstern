# Downloads any ships that need updating.
# The steps of this process are:
# + Get the current subscriber info
# + If different then stored, make note and clear LSS.force
# + Download and install all ships from LSS.force
# + Clear LSS.force
# + If the current subscriber matches what is stored:
#   - Query the server for a list of changes since the respective stored times
#   - Download and install every ship returned
# + Else,
#   - Query the server for a full list of subscribed ships
#   - Examine each ship returned; if we do not yet have it, or it has a
#     modification time later than that of the locally-stored one, download
#     and install it
#   - Update the stored subscriber info
# + Set the last download time in LSS

if {![info exists SHIPDNLD_PROCS]} {
  set SHIPDNLD_PROCS {}

  # Searches for an already-existing ship with the given guid
  # and returns its name (eg, 0/foo); otherwise, returns an empty
  # string.
  # Only ships with the given owner are examined.
  proc shipdnld_findShip {guid owner} {
    for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
      set ship [$ str hangar.all_ships.\[$i\]]
      set ship [shipName2Mount $ship]
      if {[$ exists $ship.info.guid]
      &&  $guid eq [$ str $ship.info.guid]
      &&  $owner == [$ int $ship.info.ownerid]} {
        return [$ str hangar.all_ships.\[$i\]]
      }
    }
    return {}
  }

  # Installs a ship stored in the given temporary file.
  # The fileid and owner must be indicated.
  # The temporary file will be moved to the new location.
  proc shipdnld_install {tmpfile shipname shipid fileid owner} {
    $ open $tmpfile tmpship
    set guid [$ str tmpship.info.guid]
    $ close tmpship
    set curr [shipdnld_findShip $guid $owner]
    if {"" == $curr} {
      set filename [generateNewShipBasename $shipname $owner]
      set filename [shipName2Path $filename]

      set addToHangars yes
    } else {
      set filename [shipName2Path $curr]
      # Remove current
      $ close [shipName2Mount $curr]
      file delete $filename
      set addToHangars no
    }

    # Create directory if necessary
    file mkdir [file dir $filename]
    # Install the file
    file rename $tmpfile $filename
    set ship [shipPath2Mount $filename]
    $ open $filename $ship
    catch { $ remove $ship.info.fileid }
    catch { $ remove $ship.info.ownerid }
    catch { $ remove $ship.info.needs_uploading }
    $ addi $ship.info fileid $fileid
    $ addi $ship.info ownerid $owner
    $ addb $ship.info needs_uploading no
    $ sync $ship

    if {$addToHangars} {
      $ appends hangar.all_ships [shipMount2Name $ship]
      $ appends hangar.[string tolower [$ str $ship.info.class]] [shipMount2Name $ship]
      refreshStandardHangars
      $ sync hangar
    }

    ::abnet::shipdl $shipid
  }

  set shipdnld_dc_tmpfile {}
  # Callback to download successive items in ::shipdnld_list.
  # The list must have the format
  #   shipid fileid owner name
  # It uses the ::shipdnld_listix variable to keep track of the
  # current location, but destroys ::shipdnld_list anyway. The
  # index must be set to zero when starting a new list.
  #
  # When complete, runs the callback defined in ::shipdnld_post.
  # It returns 100% if the callback does not return for it.
  proc shipdnld_downloadCallback {} {
    global shipdnld_list shipdnld_listix

    # Install any pending ships
    if {!$::abnet::busy && "" != $::shipdnld_dc_tmpfile && $::abnet::success} {
      shipdnld_install $::shipdnld_dc_tmpfile $::shipdnld_dc_shipname \
        $::shipdnld_dc_shipid $::shipdnld_dc_fileid $::shipdnld_dc_owner
      set ::shipdnld_dc_tmpfile {}
    }

    if {$::abnet::busy} {
      return [expr {$shipdnld_listix*100 / ([llength $shipdnld_list]/4+$shipdnld_listix)}]
    } elseif {!$::abnet::isConnected} {
      # Can't do anything else
      return 200
    } elseif {0 == [llength $shipdnld_list]} {
      namespace eval :: $::shipdnld_post
      return 100
    } else {
      set shipdnld_list [lassign $shipdnld_list shipid fileid owner name]
      incr shipdnld_listix

      set outf [tmpname]
      # Set parms to use once the download completes
      set ::shipdnld_dc_tmpfile $outf
      set ::shipdnld_dc_shipname $name
      set ::shipdnld_dc_shipid $shipid
      set ::shipdnld_dc_fileid $fileid
      set ::shipdnld_dc_owner $owner

      # Download the file and wait for it
      ::abnet::getf $fileid $outf
      return [expr {$shipdnld_listix*100 / ([llength $shipdnld_list]/4+$shipdnld_listix)}]
    }
  }

  # Callback to wait for ::abnet::busy to become false, then
  # evaluate ::shipdnld_post
  proc shipdnld_waitCallback {} {
    if {$::abnet::busy} {
      expr {([clock seconds]-$::abnet::lastReceive)*100/$::abnet::MAXIMUM_BUSY_TIME}
    } elseif {!$::abnet::isConnected} {
      # Nothing else can be done
      return 200
    } else {
      namespace eval :: $::shipdnld_post
      return 100 ;# In case the callback didn't return
    }
  }

  # Callback after retrieving current subscription information.
  # Determines whether the obsolete-ships or all-ships method
  # should be used.
  proc shipdnld_analyseSubscriptionInfo {} {
    if {[lssi_subscriptionMatchesRecord $::abnet::userSubscriptions $::abnet::shipSubscriptions]} {
      set ::shipdnld_post shipdnld_appendAutoObsoleteShips
      ::abnet::obsoleteShips
    } else {
      set ::shipdnld_post shipdnld_appendManualObsoleteShips
      ::abnet::subscribedShips
    }
    $::state setCallback [_ A boot shipdnld_find] shipdnld_waitCallback
  }

  # Callback after retrieving obsolete ship list.
  # Appends these ships to the ship download queue, then transitions
  # to the actual downloader.
  proc shipdnld_appendAutoObsoleteShips {} {
    # The returned list already has the proper format
    set ::shipdnld_list [concat $::shipdnld_list $::abnet::obsoleteShips]
    set ::shipdnld_post {
      lssi_clearForceList
      lssi_setDownload
      lssi_setUpload
      $ sync lssinfo
      return 200
    }
    set ::shipdnld_listix 0
    $::state setCallback [_ A boot shipdnld_main] shipdnld_downloadCallback
  }

  # Callback after retrieving full ship list.
  # The modification date of each file is checked against the local
  # copy, and is enqueued if it is later than the current, or if
  # a local copy does not exist.
  # (So that this is not an n^2 process, we first create a dict of
  #  all fileids and ship files.)
  # This clears the download list first (since the force list is
  # entirely unnecessary).
  #
  # This proc will also perform the following patches:
  # + 1/ ships are matched by name in addition to fileid
  # + If patch-info.local-migration does not exist, all local/ ships
  #   are moved to userid/.
  proc shipdnld_appendManualObsoleteShips {} {
    set ::shipdnld_list {}
    lssi_clearForceList

    set localMigrationPatch no
    if {![$ exists patch-info.local-migration]} {
      $ addb patch-info local-migration yes
      set localMigrationPatch yes
    }

    set filemap {}
    for {set ix 0} {$ix < [$ length hangar.all_ships]} {incr ix} {
      set s [$ str hangar.all_ships.\[$ix\]]
      set sm [shipName2Mount $s]
      set sf [shipName2Path $s]
      if {[$ exists $sm.info.fileid]} {
        dict set filemap [$ int $sm.info.fileid] $sf
      }

      if {$localMigrationPatch && 0 == [string first local/ $s]} {
        # Give to current user
        set ns "$::abnet::userid/[string range $s 6 end]"
        $ close $sm
        set nsf [shipName2Path $ns]
        file mkdir hangar/$::abnet::userid
        file rename $sf $nsf
        set os $s
        set s $ns
        set sf $nsf
        set sm [shipName2Mount $s]
        $ open $sf $sm

        # Update hangar
        foreach topl {all_ships c b a} {
          for {set j 0} {$j < [$ length hangar.$topl]} {incr j} {
            if {$os == [$ str hangar.$topl.\[$j\]]} {
              $ sets hangar.$topl.\[$j\] $s
            }
          }
        }
        for {set i 0} {$i < [$ length hangar.user]} {incr i} {
          set topl user.\[$i\].contents
          for {set j 0} {$j < [$ length hangar.$topl]} {incr j} {
            if {$os == [$ str hangar.$topl.\[$j\].target]} {
              $ sets hangar.$topl.\[$j\].target $s
            }
          }
        }
      }
    }

    # Sync patch info
    $ sync hangar
    $ sync patch-info

    # Scan for changes
    foreach {shipid fileid owner name modified} $::abnet::subscribedShips {
      set shipfile ""
      if {[dict exists $filemap $fileid]} {
        set shipfile [dict get $filemap $fileid]
      }
      if {"" == $shipfile} {
        set shipfile [shipdnld_findShip $fileid $name]
        if {"" != $shipfile} {
          set shipfile [shipName2Path $shipfile]
          log $shipfile
        }
      }

      if {"" == $shipfile
      ||  $modified > [file mtime $shipfile]} {
        # New or updated file
        lappend ::shipdnld_list $shipid $fileid $owner $name
      }
    }

    # Move to the next callback
    set ::shipdnld_listix 0
    set ::shipdnld_post {
      lssi_setDownload
      lssi_setUpload
      lssi_updateSubscriptionRecord $::abnet::userSubscriptions $::abnet::shipSubscriptions
      $ sync lssinfo
      return 200
    }
    $::state setCallback [_ A boot shipdnld_main] shipdnld_downloadCallback
  }

  # Sets up the initial callback (to retrieve the subscription information)
  proc shipdnld_init {} {
    if {!$::abnet::isConnected} return
    set ::shipdnld_post shipdnld_analyseSubscriptionInfo
    set ::shipdnld_list [lssi_getForceList]
    ::abnet::fetchSubscriptions
    $::state setCallback [_ A boot shipdnld_fetchsubs] shipdnld_waitCallback
  }
}

shipdnld_init
