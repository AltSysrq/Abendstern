# Uploads ships we own that have info.sharing_enabled set to
# true and info.needs_uploading to true.
# If either doesn't exist, create it and set it to true.

set ourShips {}
if {"" != $::abnet::userid} {
  for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
    set ship [$ str hangar.all_ships.\[$i\]]
    if {0 == [string first $::abnet::userid/ $ship]} {
      lappend ourShips $ship
    }
  }
}

set lastUploadedShip {}
set lastUploadedShipFN {}
if {[llength $ourShips]} {
  set ourShipIx 0
  $state setCallback [_ A boot shipupld] {
    if {"" != $lastUploadedShip && $::abnet::success && !$::abnet::busy} {
      $ seti $lastUploadedShip.info.fileid \
        $::abnet::filenames($::abnet::userid,$lastUploadedShipFN)
      $ sync $lastUploadedShip
      set lastUploadedShip {}
    }
    if {$::abnet::busy} {
      return [expr {$ourShipIx * 100 / [llength $ourShips]}]
    } elseif {$ourShipIx >= [llength $ourShips] || !$::abnet::isConnected} {
      if {$::abnet::isConnected} {
        lssi_setUpload
        $ sync lssinfo
      }
      return 200
    } else {
      # Check 16 ships per frame, barring uploads
      for {set i 0} {$i < 16 && $ourShipIx < [llength $ourShips] && !$::abnet::busy} {incr i} {
        set shipname [lindex $ourShips $ourShipIx]
        set ship [shipName2Mount $shipname]
        incr ourShipIx

        # If not loaded, assume it does not need to be uploaded
        # (This might not be correct, if Abendstern exited uncleanly and the
        # ship file was not loaded since, but this will make this step much
        # faster.)
        if {![$ loaded $ship]} continue

        if {![$ exists $ship.info.sharing_enabled]} {
          $ addb $ship.info sharing_enabled yes
        }
        if {![$ exists $ship.info.needs_uploading]} {
          $ addb $ship.info needs_uploading yes
        }

        # Possible secondary patching
        if {![$ exists $ship.info.ownerid]} {
          $ addi $ship.info ownerid $::abnet::userid
        }
        if {![$ exists $ship.info.author]} {
          $ adds $ship.info author $::abnet::username
        }
        if {![$ exists $ship.info.fileid]} {
          $ addi $ship.info fileid 0
        }
        if {![$ exists $ship.info.original_filename]} {
          $ adds $ship.info original_filename [shipName2Path $shipname]
        }
        if {$::abnet::username != [$ str $ship.info.author]} {
          $ sets $ship.info.author $::abnet::username
          $ setb $ship.info.needs_uploading yes
        }
        if {![$ exists $ship.info.guid]} {
          $ adds $ship.info guid [::uuid::uuid generate]
        }

        if {[$ bool $ship.info.needs_uploading]} {
          # Send to server
          $ sync $ship
          set filename [shipMount2Path $ship]
          set netfn "[$ str $ship.info.guid].ship"
          ::abnet::putf $netfn $filename \
              [$ bool $ship.info.sharing_enabled]
          $ setb $ship.info.needs_uploading no
          $ sync $ship
          set lastUploadedShip $ship
          set lastUploadedShipFN $netfn
        }
      }
      return [expr {$ourShipIx * 100 / [llength $ourShips]}]
    }
  }
}
