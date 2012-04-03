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

  method set-datp {key val} {
    dict set datp {*}$key $val
  }
  method set-dats {key val} {
    dict set dats {*}$key $val
  }

  method gat-peers {} {
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

  method delete-with-parent {} {
    return 1
  }
}
