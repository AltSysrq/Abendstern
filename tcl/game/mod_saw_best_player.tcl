# Provides an indication of the current best player and his score in the status
# area.
class MixinSAWBestPlayer {
  variable line

  # Creates a best player widget on the given line (default 0)
  constructor {{l 0}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    set bestScore -99999
    set best {}
    foreach peer [$this getPeers] {
      foreach vp [$this dpgp $peer list] {
        if {[$this dpgp $peer $vp score] > $bestScore} {
          set bestScore [$this dpgp $peer $vp score]
          set best "[$this getStatsFormat $peer $vp]: $bestScore"
        }
      }
    }
    lappend l $line $best
    return $l
  }
}