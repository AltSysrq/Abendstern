# Provides a status area widget to show the score of the local player's team
# and its score.
class MixinSAWLocalTeam {
  variable line

  # Creates the widget on the given line (default 1)
  constructor {{l 1}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    catch {
      set team [$this dpg 0 team]
      set score [lindex [$this dsg teamScores] $team]
      set msg "\a\[$::teamColourCode($team)$::teamName($team)\a\]: $score"
      lappend l $line $msg
    }
    return $l
  }
}
