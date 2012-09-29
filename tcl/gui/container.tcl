  # A Container is an AWidget that holds other AWidgets.
  # This class implements all functionality necessary for
  # this, EXCEPT for the setSize method (during which layout
  # must be done) the abstract getMembers method, which
  # subclasses must override to return the list of members
  # (most subclasses will also need an add method).
  # The minWidth and minHeight must also be defined by
  # subclasses.
  # The Container's destructor automatically deletes all
  # its members.
  class Container {
    inherit AWidget

    constructor {} {AWidget::constructor} {}
    destructor {
      foreach mem [getMembers] {
        delete object $mem
      }
    }

    # Returns a list of AWidgets that are contained
    # by this Container
    method getMembers {}

    # isFocused returns true if ANY of its members
    # are focused
    method isFocused {} {
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          return yes
        }
      }
      return no
    }

    # setAccelerator calls the method for all subclasses, while
    # leaving its own untouched (it has no accelerator).
    method setAccelerator p {
      foreach mem [getMembers] {
        set p [$mem setAccelerator $p]
      }
      return $p
    }

    # The default draw just calls same for all members
    method draw {} {
      foreach mem [getMembers] {
        $mem draw
      }
    }

    method drawCompiled {} {
      foreach mem [getMembers] {
        $mem drawCompiled
      }
    }

    method revert {} {
      foreach mem [getMembers] {
        $mem revert
      }
    }

    method save {} {
      foreach mem [getMembers] {
        $mem save
      }
    }

    # The event handlers just forward things to
    # members, honouring the focus requirements
    method keyboard evt {
      # First, see if anyone's focused; if so,
      # pass the event and return
      # Otherwise, tell everybody about it
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          $mem keyboard $evt
          return
        }
      }
      foreach mem [getMembers] {
        $mem keyboard $evt
      }
    }

    method character ch {
      # Only pass to focused member if there are any
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          $mem character $ch
          return
        }
      }
      foreach mem [getMembers] {
        $mem character $ch
      }
    }

    method motion {x y states} {
      foreach mem [getMembers] {
        $mem motion $x $y $states
      }
    }

    method button {evt x y} {
      foreach mem [getMembers] {
        $mem button $evt $x $y
      }
    }
  }

