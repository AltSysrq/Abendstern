# The LoopbackCommunicator is a simple Communicator that runs a simple
# single-peer system, used for non-networked games.
# This class is considered part of the containing BasicGame, and is deleted
# along with it.
class LoopbackCommunicator {
  inherit Communicator

  constructor {args} {
    Communicator::constructor {*}$args
  } {
  }

  method set-datp {keyval} {
    dict set datp 0 {*}$keyval
  }
  method set-dats {keyval} {
    dict set dats {*}$keyval
  }

  method get-peers {} {
    return 0
  }

  method is-overseer {} {
    return 1
  }

  method get-overseer {} {
    return 0
  }

  method send-broadcast {msg} {
    $game receiveBroadcast 0 $msg
  }

  method send-unicast {peer msg} {
    $game receiveUnicast 0 $msg
  }

  method send-overseer {msg} {
    $game receiveOverseer 0 $msg
  }

  method get-peer-number {peer} {
    return 0
  }

  method has-peer-by-number {num} {
    expr {$num == 0}
  }

  method get-peer-by-number {num} {
    return 0
  }

  method get-peer-by-nid {num} {
    return 0
  }

  method get-peer-nid {peer} {
    return 0
  }

  method delete-with-parent {} {
    return 1
  }

  method is-networked {} {
    return 0
  }
}
