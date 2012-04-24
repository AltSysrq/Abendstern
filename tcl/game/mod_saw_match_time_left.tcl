# Provides a status area widget to show how much time remains in the current
# match.
class MixinSAWMatchTimeLeft {
  variable line

  # Creates the widget on the given line (default 2)
  constructor {{l 2}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    set endTime [$this dsg matchEndTime]
    set timeLeft [expr {$endTime-[::abnet::nclock]}]
    if {$timeLeft > 0} {
      lappend l $line [format [_ A game time_left_fmt] \
                           [expr {$timeLeft/60}] [expr {$timeLeft%60}]]
    }
    return $l
  }
}