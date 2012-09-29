  # A StackContainer simply stacks all its components.
  # It generally should only be used to stack non-interactive
  # widgets, except for the top layer.
  class StackContainer {
    inherit Container

    variable members

    constructor {} {Container::constructor} {
      set members {}
    }

    method getMembers {} {return $members}
    method minWidth {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minWidth])}]
      }
      return $max
    }
    method minHeight {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minHeight])}]
      }
      return $max
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      foreach mem $members {
        $mem setSize $l $r $t $b
      }
    }

    method add w {
      lappend members $w
    }
  }
