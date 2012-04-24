# Provides a status area widget which indicates how many players are currently
# alive.
class MixinSAWPlayersLeft {
  variable line

  # Creates the widget on the given line (default 2)
  constructor {{l 2}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]

    set cnt 0
    foreach peer [$this getPeers] {
      foreach vp [$this dpgp $peer list] {
        if {[$this dpgp $peer $vp alive]} {
          incr cnt
        }
      }
    }

    lappend l $line [format [_ A game players_left_fmt] $cnt]
    return $l
  }
}
