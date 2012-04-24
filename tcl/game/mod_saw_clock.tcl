# BasicGame module that adds a wall clock to the status area elements.
class MixinSAWClock {
  variable line

  # Constructs a clock on the given line (default 3)
  constructor {{l 3}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    lappend l $line [clock format [clock seconds] -format "%H:%M:%S"]
    return $l
  }
}
