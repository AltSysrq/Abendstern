# Allows sharing indices between ShipChoosers, as well as notifying them of
# updates and such.
#
# (Note that most of the logic is internal to ShipChooser right now; this
# simply is a hub for them to share data and an external endpoint for
# deletion/creation notification).
namespace eval ship_index {
  variable fullList {}
  variable hangarsIndexed {}
  variable choosers {}

  # Adds the ship at the given mount point to all choosers, and makes it the
  # main preferred if requested.
  proc add {root makePreferred} {
    global ::ship_index::choosers
    if {$makePreferred} {
      $ sets conf.preferred.main $root
    }
    if {$choosers ne {}} {
      [lindex $choosers 0] add-mount $root

      foreach chooser $choosers {
        $chooser filter-changed
      }
    } else {
      # Let it get rediscovered later
      set ::ship_index::fullList {}
      set ::ship_index::hangarsIndexed {}
    }
  }

  # Removes the ship at the given mount point from all choosers.
  proc del {root} {
    global ::ship_index::fullList ::ship_index::choosers
    for {set ix 0} {$ix < [llength $fullList]} {incr ix} {
      if {$root eq [dict get [lindex $fullList $ix] root]} {
        set fullList [lreplace $fullList $ix $ix]
        foreach chooser $choosers {
          $chooser filter-changed
        }
      }
    }
  }
}
