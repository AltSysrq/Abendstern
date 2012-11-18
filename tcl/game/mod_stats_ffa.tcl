# This is a mixin for game modes without team focus, providing
# a stats page sorted by individual user score.
# Requires a method getGameModeDescription, which is used as
# the title for the stats page.

class MixinStatsFFA {
  method getStatsPanel {} {
    set stats {} ;# Display name, score
    # Gather lines to display
    set appendOverseer no
    foreach peer [$this getPeers] {
      if {$peer == [$this getOverseer]} {
        set appendOverseer yes
      }
      foreach vpeer [$this dpgp $peer list] {
        set name [$this dpgp $peer $vpeer name]
        set score [$this dpgp $peer $vpeer score]
        set name "\a\[[$this getStatsColour $peer $vpeer]$name\a\]"
        # Append alive status
        if {[$this dpgp $peer $vpeer alive]} {
          set name "\a\[(special)+\a\]$name"
        } else {
          set name "\a\[(danger)-\a\]$name"
        }
        if {$appendOverseer} {
          set appendOverseer no
          append name " \a{\a\[(white)!\a\]\a}"
        }
        set score [expr {int($score)}]
        lappend stats [list $name $score]
      }
    }
    set stats [lsort -integer -decreasing -index 1 $stats]

    # Don't show more than 48
    set stats [lrange $stats 0 47]

    # Determine number of columns (at least 1, max 3,
    # prefer 12/column)
    set ncolumns [expr {min(3,max(1,([llength $stats]+6)/12))}]

    set top [new ::gui::BorderContainer 0 0.01]
    set main [new ::gui::HorizontalContainer 0.02 grid]
    set ix 0
    for {set col 0} {$col < $ncolumns} {incr col} {
      set percol [expr {[llength $stats]/$ncolumns}]
      if {$col < [llength $stats]%$ncolumns} {
        # Distribute the remainder over the left-most columns
        incr percol
      }

      set vpan [new ::gui::VerticalContainer 0.002]
      for {set row 0} {$row < $percol} {incr row; incr ix} {
        set rpan [new ::gui::BorderContainer]
        $rpan setElt centre [new ::gui::Label [lindex $stats $ix 0] left]
        $rpan setElt right [new ::gui::Label " [lindex $stats $ix 1]" right]
        $vpan add $rpan
      }

      #$main add [new ::gui::Frame $vpan]
      $main add $vpan
    }

    $top setElt centre $main
    set timeLeftNote {}
    catch {
      set endTime [$this dsg matchEndTime]
      set timeLeft [expr {$endTime-[::abnet::nclock]}]
      if {$timeLeft >= 0 && [$this isMatchTimed]} {
        set timeLeftNote \
          [format ": [_ A game time_left_fmt]" \
            [expr {$timeLeft/60}] [expr {$timeLeft%60}]]
      }
    }
    $top setElt top \
      [new ::gui::Label "[$this getGameModeDescription]$timeLeftNote" centre]
    $top setElt bottom [new ::gui::Label [_ A game stats_overseer_note] left]

    return $top
  }
}
