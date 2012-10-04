# Deletes obsolete (now non-existent or non-shared) ships from the server

if {$::abnet::isConnected} {
  set ourShips {}
  if {"" != $::abnet::userid} {
    for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
      set ship [$ str hangar.all_ships.\[$i\]]
      if {0 == [string first $::abnet::userid/ $ship]} {
        lappend ourShips "[spc::get [shipName2Mount $ship] guid].ship"
      }
    }

    ::abnet::shipfls
  }
  set ourShips [lsort $ourShips]

  $state setCallback [_ A boot shipdelo] {
    if {!$::abnet::busy} {
      set shipflistix 0
      $state setCallback [_ A boot shipdelo] {
        for {set i 0} {$i < 16 &&
                       $::abnet::isConnected && !$::abnet::busy &&
                       $shipflistix < [llength $::abnet::shipflist]} {incr i} {
          set fileid [lindex $::abnet::shipflist $shipflistix]
          incr shipflistix
          set filename [lindex $::abnet::shipflist $shipflistix]
          incr shipflistix

          # Do we still share this file?
          if {-1 == [lsearch -exact -sorted $ourShips $filename]} {
            # No, remove it from the server
            log "Delete $filename ($fileid)"
            ::abnet::rmf $fileid
          }
        }

        if {$::abnet::isConnected
        && ($::abnet::busy ||  $shipflistix < [llength $::abnet::shipflist])} {
          return [expr {$shipflistix*100 / [llength $::abnet::shipflist]}]
        } else {
          return 200
        }
      }
    }
    return 0
  }
}
