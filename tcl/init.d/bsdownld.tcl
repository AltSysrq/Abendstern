# Downloads all current AI best ships
# Only does downloading at most once per day

if {![info exists ::bsdownld::index]} {
  namespace eval bsdownld {
    set index {}

    set filenames {}
    foreach class {A B C} {
      foreach cat {0 1 2 3 4 5} {
        foreach ix {0 1 2 3 4} {
          lappend filenames "bestship$class$cat$ix"
        }
      }
    }

    proc callback {} {
      global ::bsdownld::index
      global ::bsdownld::filenames

      if {!$::abnet::isConnected} {
        # Can't do anything else
        return 200
      }

      if {!$::abnet::busy} {
        if {$index >= [llength $filenames]} {
          return 200 ;# Done
        }

        set filename [lindex $filenames $index]
        ::abnet::getfn $filename [homeq hangar/0/$filename] 0
        incr index
      }

      return [expr {int(100.0 * $index / [llength $filenames])}]
    }
  }

  if {[clock seconds] > [lssi_getBestship]+3600*24 && $::abnet::isConnected} {
    set ::bsdownld::index 0
    file mkdir [homeq hangar/0]
    $state setCallback [_ A boot bsdownld] ::bsdownld::callback
    lssi_setBestship
  }
}
