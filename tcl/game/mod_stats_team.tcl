# Mixin which implements a stats panel for teams.
# It uses (but doesn't modify) the same global dats as mod_team.

class MixinStatsTeam {
  method getStatsPanel {} {
    # Use layouts as follows:
    #   No team         Return empty
    #   1 team          X
    #
    #   2 teams         X X
    #
    #   3 teams         X X
    #                    X
    #   4 teams         X X
    #                   X X
    #   5 teams         XXX
    #                   X X
    #   6 teams         XXX
    #                   XXX

    # First, accumulate the extant team indices and their scores
    set teams {}
    set teamExistence [$this dsg teams]
    for {set i 0} {$i < [llength $teamExistence]} {incr i} {
      if {[lindex $teamExistence $i]} {
        lappend teams [list $i [lindex [$this dsg teamScores] $i]]
      }
    }

    set teams [lsort -decreasing -integer -index 1 $teams]

    switch [llength $teams] {
      0 { set rowu 0; set rowl 0 }
      1 { set rowu 1; set rowl 0 }
      2 { set rowu 2; set rowl 0 }
      3 { set rowu 3; set rowl 0 }
      4 { set rowu 2; set rowl 2 }
      5 { set rowu 3; set rowl 2 }
      6 { set rowu 3; set rowl 3 }
    }

    set top [new ::gui::BorderContainer 0 0.01]
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
    set main [new ::gui::VerticalContainer 0.02]
    $top setElt centre $main

    set upper [new ::gui::HorizontalContainer 0.02 centre]
    $main add $upper
    if {$rowl > 0} {
      set lower [new ::gui::HorizontalContainer 0.02 centre]
      $main add $lower
    }

    set teamix 0
    for {set i 0} {$i < $rowu} {incr i} {
      $upper add [getStatsSubPanel [lindex $teams $teamix 0]]
      incr teamix
    }
    for {set i 0} {$i < $rowl} {incr i} {
      $lower add [getStatsSubPanel [lindex $teams $teamix 0]]
      incr teamix
    }

    return $top
  }

  private method getStatsSubPanel team {
    set pan [new ::gui::VerticalContainer 0.01]
    set p [new ::gui::BorderContainer]
    $p setElt centre [new ::gui::Label "\a\[$::teamColourCode($team)$::teamName($team)\a\]" left]
    $p setElt right [new ::gui::Label \
      " \a\[$::teamColourCode($team)[lindex [$this dsg teamScores] $team]\a\]" right]
    $pan add $p

    # Find the members of the team
    set members {} ;# Display name, score
    set appendOverseer no
    foreach peer [$this getPeers] {
      foreach vpeer [$this dpgp $peer list] {
        if {$team == [$this dpgp $peer $vpeer team]} {
          set name [$this dpgp $peer $vpeer name]
          set score [$this dpgp $peer $vpeer score]
          set name "\a\[[$this getStatsColour $peer $vpeer]$name\a\]"
          # Append alive status
          if {[$this dpgp $peer $vpeer alive]} {
            set name "\a\[(special)+\a\]$name"
          } else {
            set name "\a\[(danger)-\a\]$name"
          }
          if {$peer == [$this getOverseer] && $vpeer == 0} {
            append name " \a{\a\[(white)!\a\]\a}"
          }
          set score [expr {int($score)}]
          lappend members [list $name $score]
        }
      }
    }

    set members [lsort -decreasing -integer -index 1 $members]
    set members [lrange $members 0 11] ;# Limit to 12 members

    foreach item $members {
      set p [new ::gui::BorderContainer]
      $p setElt centre [new ::gui::Label [lindex $item 0] left]
      $p setElt right [new ::gui::Label " [lindex $item 1]" right]
      $pan add $p
    }

    return $pan
  }
}
