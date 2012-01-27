# Provides a GUI for the user to view other users' ships and edit
# their subscriptions.
class ShipBrowser {
  inherit ::gui::Application

  variable isAlive

  constructor {} {
    ::gui::Application::constructor
  } {
    set isAlive yes
    set mode [new ShipBrowserMode $this]
  }

  method updateThis et {
    ::sbg::update $et
    $mode update $et
    if {!$::abnet::isConnected} {
      return [new BootManager login]
    } elseif {$isAlive} {
      return 0
    } else {
      return [new BootManager refreshShips]
    }
  }

  method drawThis {} {
    ::sbg::draw
  }

  method done {} {
    set isAlive no
  }

  method getCursor {} {
    if {$::abnet::busy} {
      return busy
    } else {
      return crosshair
    }
  }
}

class ShipBrowserMode {
  inherit ::gui::Mode

  variable app

  variable currentAction                ;# Determines what to do after not busy
  variable rsortAlpha                   ;# If checked, sort users alphabetically
  variable rsortNShips                  ;# If checked, sort users by ship count
  variable rsortPop                     ;# If checked, sort users by popularity
  variable rsortRate                    ;# If checked, sort users by average rating
  variable resetPage                    ;# If true, reset to page 0 next frame
  variable lists                        ;# TapPanel that contains both lists
  variable lusers                       ;# List of users on the current page
  variable userList                     ;# Lists user names and userids
  variable lblpage                      ;# Current page number
  variable currPage                     ;# Current page number
  variable lblusername                  ;# The name of the current user
  variable iavatar                      ;# Avatar of current user
  variable lships                       ;# List of ships owned by current user
  variable shipList                     ;# Lists ship {displayName, name, shipid, fileid, class}s
  variable lblshipname                  ;# The name of the ship
  variable lblrating                    ;# Rating of current ship
  variable lbldownloads                 ;# Number of times current ship downloaded
  variable svship                       ;# Display current ship
  variable cuserSubscribe               ;# Whether to subscribe to the current user
  variable cshipSubscribe               ;# Whether to subschibe to the current ship

  variable ssgGraph                     ;# ShipSpiderGraph

  variable tmpfile                      ;# Filename to save ships to
  variable loadedShip                   ;# Temporary ship loaded
  variable gamefield                    ;# Scratch field to load the ship into
  variable forceByUser                  ;# Dict mapping userids to ship lists (for lssi)
  variable forceByShip                  ;# Dict mapping shipids to ship lists (for lssi)

  constructor sb {
    ::gui::Mode::constructor
  } {
    set app $sb
    set tmpfile [tmpname]
    set gamefield [new GameField default 1 1]
    set loadedShip {}
    set resetPage no

    set main [new ::gui::BorderContainer]
    $main setElt top [new ::gui::Label [_ A shipbrowse title] centre]
    set listsBoth [new ::gui::BorderContainer]
    set lists [new ::gui::TabPanel]
    $main setElt left $listsBoth
    set userListPanel [new ::gui::BorderContainer]
    $lists add [_ A shipbrowse users] $userListPanel
    set shipListPanel [new ::gui::BorderContainer]
    $lists add [_ A shipbrowse ships] $shipListPanel
    $listsBoth setElt centre $lists
    set cuserSubscribe [new ::gui::Checkbox [_ A shipbrowse subscribe_user] \
       {} {} "$this userSubscribe" "$this userUnsubscribe"]
    $listsBoth setElt bottom $cuserSubscribe

    set userListOptions [new ::gui::VerticalContainer 0.01]
    $userListPanel setElt top $userListOptions
    $userListOptions add [new ::gui::Label [_ A shipbrowse sorting] left]
    set rsortAlpha [new ::gui::RadioButton [_ A shipbrowse sortalpha] {expr 1} {} \
                    none "$this resetUL"]
    $userListOptions add $rsortAlpha
    set rsortNShips [new ::gui::RadioButton [_ A shipbrowse sortnships] {} {} \
                     $rsortAlpha "$this resetUL"]
    $userListOptions add $rsortNShips
    set rsortPop [new ::gui::RadioButton [_ A shipbrowse sortpop] {} {} \
                  $rsortNShips "$this resetUL"]
    $userListOptions add $rsortPop
    set rsortRate [new ::gui::RadioButton [_ A shipbrowse sortrate] {} {} \
                   $rsortPop "$this resetUL"]
    $userListOptions add $rsortRate
    set lusers [new ::gui::List [_ A shipbrowse userlist] {} no "$this userchange"]
    $userListPanel setElt centre $lusers
    set userListPageChange [new ::gui::BorderContainer]
    set userListPageLeft [new ::gui::Button << "$this page -1"]
    $userListPageLeft setLeft
    set userListPageRight [new ::gui::Button >> "$this page +1"]
    $userListPageRight setRight
    set lblpage [new ::gui::Label 1 centre]
    $userListPageChange setElt left $userListPageLeft
    $userListPageChange setElt centre $lblpage
    $userListPageChange setElt right $userListPageRight
    $userListPanel setElt bottom $userListPageChange

    set avatarPanel [new ::gui::BorderContainer 0 0.01]
    set lblusername [new ::gui::Label "" left]
    $avatarPanel setElt centre $lblusername
    set iavatar [new ::gui::SquareIcon]
    $avatarPanel setElt left [new ::gui::ComfyContainer $iavatar 0.98]
    $shipListPanel setElt top $avatarPanel
    set lships [new ::gui::List [_ A shipbrowse shiplist] {} no "$this shipchange"]
    $shipListPanel setElt centre $lships
    $iavatar dupeMinHeight $lblusername

    set shipPanel [new ::gui::BorderContainer]
    $main setElt centre $shipPanel
    set shipHeaderPanel [new ::gui::VerticalContainer 0.01]
    set lblshipname [new ::gui::Label {} centre]
    $shipHeaderPanel add $lblshipname
    set shipInfoPanel [new ::gui::HorizontalContainer 0 grid]
    set ssgGraph [new ::gui::ShipSpiderGraph]
    $shipInfoPanel add $ssgGraph
    set rdpanel [new ::gui::VerticalContainer 0.01]
    set lblrating [new ::gui::Label "" left]
    set lbldownloads [new ::gui::Label "" left]
    $rdpanel add $lblrating
    $rdpanel add $lbldownloads
    $shipInfoPanel add $rdpanel
    $shipHeaderPanel add $shipInfoPanel
    $shipPanel setElt top $shipHeaderPanel
    set svship [new ::gui::SimpleShipDisplay]
    $shipPanel setElt centre $svship
    set shipControls [new ::gui::VerticalContainer 0.01]
    set cshipSubscribe [new ::gui::Checkbox [_ A shipbrowse subscribe_ship] \
                        {} {} "$this shipSubscribe" "$this shipUnsubscribe"]
    $shipControls add $cshipSubscribe
    set buttons [new ::gui::BorderContainer]
    set recommendationsPanel [new ::gui::HorizontalContainer 0.01 left]
    $recommendationsPanel add [new ::gui::Label [_ A shipbrowse shiprate] left]
    $recommendationsPanel add [new ::gui::Button [_ A shipbrowse shiprate+] "$this rate 1"]
    $recommendationsPanel add [new ::gui::Button [_ A shipbrowse shiprate-] "$this rate 0"]
    $buttons setElt centre $recommendationsPanel
    set can [new ::gui::Button [_ A gui close] "$app done"]
    $can setCancel
    $buttons setElt right $can
    $shipControls add $buttons
    $shipPanel setElt bottom $shipControls

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    set forceByShip {}
    set forceByUser {}

    setPage 0
  }

  destructor {
    file delete $tmpfile
    deleteShip
    delete object $gamefield
    dict for {userid data} $forceByUser {
      lssi_appendForceList $data
    }
    dict for {shipid data} $forceByShip {
      lssi_appendForceList $data
    }
    lssi_updateSubscriptionRecord $::abnet::userSubscriptions $::abnet::shipSubscriptions
  }

  method deleteShip {} {
    if {"" != $loadedShip && "error" != $loadedShip} {
      delete object $loadedShip
    }
    set loadedShip {}
  }

  method update et {
    if {$resetPage} {
      setPage 0
      set resetPage no
    }
    if {!$::abnet::busy && "" != $currentAction} {
      set ca $currentAction
      set currentAction {}
      finish-$ca
    }
  }

  method resetUL {} {
    set resetPage yes
  }

  # Returns a datum from the user list, for the user at
  # the given index. The datum may be userid or name
  method getFromUsers {datum ix} {
    switch -exact -- $datum {
      userid {set sub 1}
      name {set sub 0}
    }
    lindex $userList [expr {$ix*2+$sub}]
  }

  # Returns a datum from the ship list, for the ship at
  # the given index. The datum may be fileid, shipid, name, class, or displayName
  method getFromShips {datum ix} {
    switch -exact -- $datum {
      displayName {set sub 0}
      name   {set sub 1}
      shipid {set sub 2}
      fileid {set sub 3}
      class  {set sub 4}
    }
    lindex $shipList [expr {$ix*5+$sub}]
  }

  method setPage p {
    # Reset everything depending on user and ship selection
    $iavatar unload
    $svship setShip {}
    deleteShip
    $lships setItems {}
    $lusers setItems {}
    $lblusername setText {}
    $lblshipname setText {}
    $lbldownloads setText {}
    $lblrating setText {}
    $cuserSubscribe setCheckedNoAction no
    $cshipSubscribe setCheckedNoAction no
    # Update GUI
    set currPage $p
    $lblpage setText [expr {$p+1}]

    # Begin request
    set currentAction get-user-list
    # Determine how many items fit on a page (subtract 1 for the label at the top)
    set n [expr {int([$lusers getHeight]/[$::gui::font getHeight])-1}]
    if {[$rsortAlpha isChecked]} {
      set method alphabetic
    } elseif {[$rsortNShips isChecked]} {
      set method numships
    } elseif {[$rsortPop isChecked]} {
      set method popularity
    } else {
      set method rating
    }
    ::abnet::userls $method [expr {$p*$n}] [expr {($p+1)*$n}]
  }
  method finish-get-user-list {} {
    set userList {}
    set display {}
    foreach {userid name} $::abnet::userlist {
      lappend userList $name $userid
      lappend display $name
    }
    $lusers setItems $display
  }

  method page diff {
    if {$currPage+$diff >= 0} {
      setPage [expr {$currPage + $diff}]
    }
  }
  method rate good {
    set sel [$lships getSelection]
    if {{} == [llength $sel] || {} == $loadedShip} return
    set shipid [getFromShips shipid $sel]
    ::abnet::shiprate $shipid $good
  }

  method userSubscribe {} {
    set sel [$lusers getSelection]
    if {{} == [llength $sel]} return
    set userid [getFromUsers userid $sel]
    ::abnet::subscribeUser $userid

    set forces {}
    foreach {displayName name shipid fileid class} $shipList {
      lappend forces $shipid $fileid $userid $name
    }
    dict set forceByUser $userid $forces
  }
  method userUnsubscribe {} {
    set sel [$lusers getSelection]
    if {{} == [llength $sel]} return
    set userid [getFromUsers userid $sel]
    ::abnet::unsubscribeUser $userid
    set forceByUser [dict remove $forceByUser $userid]
  }
  method shipSubscribe {} {
    set sel [$lships getSelection]
    if {{} == [llength $sel] || {} == $loadedShip} return
    set shipid [getFromShips shipid $sel]
    ::abnet::subscribeShip $shipid
    dict set forceByShip $shipid [list $shipid [getFromShips fileid $sel] \
      [getFromUsers userid [$lusers getSelection]] [getFromShips name $sel]]
  }
  method shipUnsubscribe {} {
    set sel [$lships getSelection]
    if {{} == [llength $sel] || {} == $loadedShip} return
    set shipid [getFromShips shipid $sel]
    ::abnet::unsubscribeShip $shipid
    set forceByShip [dict remove $forceByShip $shipid]
  }

  method userchange {} {
    set sel [$lusers getSelection]
    if {{} == $sel} return

    # Reset GUI elements affected by the change
    $iavatar unload
    $lships setItems {}
    $svship setShip {}
    $lbldownloads setText {}
    $lblrating setText {}
    $lblshipname setText {}
    $cshipSubscribe setChecked no
    deleteShip

    set userid [getFromUsers userid $sel]
    $lblusername setText [getFromUsers name $sel]

    # Update the status of the user subscription checkbox
    $cuserSubscribe setCheckedNoAction \
      [expr {-1 != [lsearch -exact $::abnet::userSubscriptions $userid]}]

    # Begin fetching the avatar
    set currentAction fetch-avatar
    ::abnet::getfn avatar $tmpfile $userid
  }
  method finish-fetch-avatar {} {
    # If successful, load the avatar
    if {$::abnet::success} {
      $iavatar load $tmpfile 64 SILRLax
    }

    if {![$iavatar isLoaded]} {
      $iavatar load images/avatar.png 64 SILRStrict
    }

    # Get the user's ship list
    set currentAction fetch-ship-list
    ::abnet::shipls [getFromUsers userid [$lusers getSelection]]
  }
  method finish-fetch-ship-list {} {
    if {!$::abnet::success} return

    # Place into subslists so we can use lsort (and create a display
    # name to sort by which prefixes the class)
    set source {}
    foreach {shipid fileid class name} $::abnet::shiplist {
      lappend source [list "$class: $name" $name $shipid $fileid $class]
    }
    set source [lsort -index 0 -dictionary $source]
    # Reformat list
    set shipList {}
    foreach data $source {
      lappend shipList {*}$data
    }

    # Strip names for display
    set display {}
    foreach {displayName name shipid fileid class} $shipList {
      lappend display $displayName
    }
    $lists setActive 1 ;# Must be set first so lships gets a size
    $lships setItems $display
  }

  method shipchange {} {
    set sel [$lships getSelection]
    if {{} == $sel} return

    set shipid [getFromShips shipid $sel]

    # Clear any GUI components affected by the change
    $svship setShip ""
    $lblrating setText {}
    $lbldownloads setText {}
    $lblshipname setText {}
    deleteShip

    # Update ship subscription checkbox to match
    $cshipSubscribe setCheckedNoAction \
      [expr {-1 != [lsearch -exact $::abnet::shipSubscriptions $shipid]}]

    # Download the ship
    set currentAction download-ship
    ::abnet::getf [getFromShips fileid $sel] $tmpfile
  }
  method finish-download-ship {} {
    # Try to load the ship
    if {[catch {
      if {!$::abnet::success} {
        error "Could not download ship"
      }
      $ open $tmpfile tmpship
      set loadedShip [loadShip $gamefield tmpship]
      set ship $loadedShip
      $lblshipname setText \
        "[$ str tmpship.info.name] ([_ A editor class_prefix][$ str tmpship.info.class])"
    } err]} {
      $lblshipname setText "\a\[(danger)[_ A gui error]\a\]"
      log $err
      set ship error
    }
    catch { $ close tmpship }

    if {{} != $loadedShip} {
      # Update the spider graph
      $ssgGraph setShip $ship
    }

    # Update the ship viewer
    $svship setShip $ship

    # Get the download/rating information
    set currentAction get-ship-info
    ::abnet::getShipInfo [getFromShips shipid [$lships getSelection]]
  }
  method finish-get-ship-info {} {
    set shipid [getFromShips shipid [$lships getSelection]]
    if {$::abnet::success} {
      $lbldownloads setText [format [_ A shipbrowse downloads_fmt] \
                                    $::abnet::shipinfo($shipid,downloads)]
      set sr $::abnet::shipinfo($shipid,sumrating)
      set nr $::abnet::shipinfo($shipid,numrating)
      if {$nr == 0} {
        set rating {}
      } else {
        # Determine a 0..5 (inclusive) rating.
        # Multiply by 5.1 instead of 5.0 so that "perfect" does not need
        # an absolutely perfect score (ie, so that one person cannot single-
        # handedly demote a perfect rating)
        set ratix [expr {int(5.1*$sr/double($nr))}]
        set rating [_ A shipbrowse rating$ratix]
      }
      $lblrating setText [format [_ A shipbrowse rating_fmt] $rating $nr]
    }
  }
}
