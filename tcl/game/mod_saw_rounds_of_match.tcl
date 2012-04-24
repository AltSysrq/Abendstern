# Provides a status area widget indicating the current round
class MixinSAWRoundsOfMatch {
  variable line

  # Creates the widget on the given line (default 2)
  constructor {{l 2}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    if {[$this dsg currentRound] < $::ROUNDS_IN_MATCH} {
      set rnd [format [_ A game round_fmt] \
                  [expr {[$this dsg currentRound]+1}] $::ROUNDS_IN_MATCH]
      lappend l $line $rnd
    }
    return $l
  }
}
