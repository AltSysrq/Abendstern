# Provides a status area widget which shows the best team and its score
class MixinSAWBestTeam {
  variable line

  # Creates the widget on the specified line (default 0)
  constructor {{l 0}} {
    set line 0
  }

  method getStatusAreaElements {} {
    set l [chain]
    set bestScore -999999
    set best ""
    set ix 0
    foreach score [$this dsg teamScores] {
      if {$score > $bestScore && [lindex [$this dsg teams] $ix]} {
        set bestScore $score
        set best "\a\[$::teamColourCode($ix)$::teamName($ix)\a\]: $score"
      }
      incr ix
    }

    lappend l $line $best
  }
}
