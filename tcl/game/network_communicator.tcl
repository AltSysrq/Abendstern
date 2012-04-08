# Communicator and NetIface which bridges NetworkGame and BasicGame.
class NetworkCommunicator {
  inherit NetIface Communicator

  variable network
  variable peers
  variable peersByNumber
  variable peersByNumberRev
  variable freeNumbers
  variable mgr
  variable currMode {}

  constructor {netw man gam} {
    super NetIface *default
    Communicator::constructor $gam
  } {
    set network $gam
    set mgr $man
    set peers [list 0]
    set peersByNumber [dict create 0 0]
    set peersByNumberRev [dict create 0 0]
    set freeNumbers {}
    for {set i 1} {$i < 256} {incr i} {
      lappend freeNumbers $i
    }
  }

  ### BEGIN: Communicator methods
  method set-datp {keyval} {
    dict set datp 0 {*}$keyval
    $network alterDatp $keyval
  }

  method set-dats {keyval} {
    dict set dats {*}$keyval
    $network alterDats $keyval
  }

  method get-peers {} {
    return $peers
  }

  method is-overseer {} {
    expr {0 == [$network getOverseer]}
  }

  method get-overseer {} {
    $network getOverseer
  }

  method send-broadcast {msg} {
    $game receiveBroadcast 0 $msg
    $network sendBroadcast $msg
  }

  method send-unicast {peer msg} {
    if {0 == $peer} {
      $game receiveUnicast 0 $msg
    } else {
      $network sendUnicast $msg $peer
    }
  }

  method send-overseer {msg} {
    if {[is-overseer]} {
      $game receiveOverseer 0 $msg
    } else {
      $network sendOverseer $msg [get-overseer]
    }
  }

  method get-peer-number {peer} {
    dict get $peersByNumberRev $peer
  }

  method has-peer-by-number {num} {
    dict exists $peersByNumber $num
  }

  method get-peer-by-number {num} {
    dict get $peersByNumber $num
  }

  method is-networked {} {
    return 1
  }

  method switchMode {mode} {
    set currMode $mode
    foreach peer $peers {
      if {$peer != 0} {
        $network sendGameMode $peer
      }
    }
  }
  ### END: Communicator methods

  ### BEGIN: NetIface functions
  method addPeer {peer} {
    lappend peers $peer
    set freeNumbers [lassign $freeNumbers num]
    dict set peersByNumber $num $peer
    dict set peersByNumberRev $peer $num
  }

  method delPeer {peer} {
    set num [dict get $peersByNumberRev $peer]
    set peers [lsearch -not -all -inline -exact $peers $peer]
    dict unset peersByNumber $num
    dict unset peersByNumberRev $peer
  }

  method setOverseer {} {}
  method receiveBroadcast {peer msg} {
    $game receiveBroadcast $peer $msg
  }
  method receiveOverseer {peer msg} {
    $game receiveOverseer $peer $msg
  }
  method receiveUnicast {peer msg} {
    $game receiveUnicast $peer $msg
  }

  method alterDats {keyval} {
    if {![string is list -strict $keyval]} {
      log "Warning: Rejecting non-list alteration of dats"
      return 0
    }
    set new $dats
    if {[catch {
      dict set new {*}$keyval
      if {![validateDats new]} {
        error "Validation failed."
      }
    } err]} {
      log "Warning: Rejecting dats alteration: $err"
      return 0
    }

    set dats $new
    return 1
  }

  method alterDatp {peer keyval} {
    if {![string is list -strict $keyval]} {
      log "Warning: Rejecting non-list alteration of datp"
      return 0
    }

    set new [dict get $datp $peer]
    if {[catcn {
      dict set new {*}$keyval
      if {![validateDatp new]} {
        error "Validation failed."
      }
    } err]} {
      log "Warning: Rejecting datp alteration: $err"
      return 0
    }

    dict set datp $peer $new
    return 1
  }

  method getFullDats {} {
    list $dats
  }

  method setGameMode {mode} {
    $mgr remote-swich-state $mode
  }

  method getGameMode {} {
    return $currMode
  }

  method connectionLost {why} {
    log "Connection lost: $why"
  }
  ### END: NetIface functions
}
