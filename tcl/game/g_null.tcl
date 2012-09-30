# Provides the NULL "game mode" which indicates that we are waiting on a
# response and gives the user the opportunity to cancel.
class NullNetworkState {
  inherit gui::Application

  # Not used; for compatibility with BasicGame
  public variable gameClass C

  variable alive yes

  constructor {netw} {
    ::gui::Application::constructor
  } {
    set mode [new NullNetworkMode $this $netw]
  }

  method cancel {} {
    set alive no
  }

  method updateThis et {
    if {$alive} {
      return 0
    } else {
      return $this
    }
  }

  # Do-nothing methods that may be called by the NetworkCommunicator
  foreach meth {
    receiveBroadcast
    receiveUnicast
    receiveOverseer
    modifyIncomming
  } {
    method $meth args {}
  }
}

class NullNetworkMode {
  inherit gui::Mode

  variable netw
  variable app

  constructor {a n} {
    Mode::constructor
  } {
    set netw $n
    set app $a

    set main [new gui::BorderContainer 0 0.01]
    set earth [new gui::SquareIcon]
    $earth load images/sol.png 256 SILRScale
    $earth dupeMinWidth 0.08
    $main setElt left $earth
    set cent [new gui::SquareIcon]
    $cent load images/acen.png 256 SILRScale
    $cent dupeMinWidth 0.08
    $main setElt right $cent
    $main setElt centre [new gui::Label [_ N network waiting] centre]
    set buttons [new gui::HorizontalContainer 0.01 right]
    set can [new gui::Button [_ A gui cancel] "$app cancel"]
    $can setCancel
    $buttons add $can
    $main setElt bottom $buttons
    set root [new ::gui::ComfyContainer $main]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
    $root pack

    # Create global dats and datp procs which always pass
    namespace eval :: {
      proc validateDats {args} {
        return 1
      }
      proc validateDatp {args} {
        return 1
      }
    }
  }
}
