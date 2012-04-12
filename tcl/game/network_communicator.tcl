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

  # Mirrors of the corresponding dicts, but not passed through the
  # validator. This allows for non-atomic operations to work properly.
  variable unvalidatedDatp
  variable unvalidatedDats

  constructor {netw man gam} {
    super NetIface *default
    Communicator::constructor $gam
  } {
    set network $netw
    set mgr $man
    set peers [list 0]
    set peersByNumber [dict create 0 0]
    set peersByNumberRev [dict create 0 0]
    set freeNumbers {}
    for {set i 1} {$i < 256} {incr i} {
      lappend freeNumbers $i
    }

    set unvalidatedDatp $datp
    set unvalidatedDats $dats
  }

  ### BEGIN: Communicator methods
  method set-datp {keyval} {
    dict set datp 0 {*}$keyval
    dict set unvalidatedDats 0 {*}$keyval
    $network alterDatp $keyval 0
  }

  method set-dats {keyval} {
    dict set dats {*}$keyval
    dict set unvalidatedDats {*}$keyval
    $network alterDats $keyval 0
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
    if {[is-overseer]} {
      foreach peer $peers {
        if {$peer != 0} {
          $network sendGameMode $peer
        }
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
    if {[dict exists $datp 0]} {
      $network alterDatp [list [dict get $datp 0]] $peer
    }
    set dp {}
    validateDatp dp
    dict set datp $peer $dp
    dict set unvalidatedDatp $peer {}
  }

  method delPeer {peer} {
    set num [dict get $peersByNumberRev $peer]
    set peers [lsearch -not -all -inline -exact $peers $peer]
    dict unset peersByNumber $num
    dict unset peersByNumberRev $peer
    dict unset datp $peer
    dict unset unvalidatedDatp $peer
  }

  method setOverseer {peer} {}
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

    if {[catch {
      if {[llength $keyval] > 1} {
        dict set unvalidatedDats {*}$keyval
      } else {
        set unvalidatedDats [lindex $keyval 0]
      }
      set new $unvalidatedDats
      if {![validateDats new]} {
        error "Validation failed."
      }
    } err]} {
      #log "Warning: Rejecting dats alteration: $err"
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

    set new [dict get $unvalidatedDatp $peer]
    if {[catch {
      if {[llength $keyval] > 1} {
        dict set new {*}$keyval
      } else {
        set new [lindex $keyval 0]
      }
      dict set unvalidatedDatp $peer $new
      if {![validateDatp new]} {
        error "Validation failed."
      }
    } err]} {
      #log "Warning: Rejecting datp alteration: $err"
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
    if {![validateDats dats]} {
      log $dats
      $mgr networkError [_ N network protocol_error]
      return
    }

    foreach peer $peers {
      if {$peer != 0} {
        set d [dict get $unvalidatedDatp $peer]
        validateDatp d
        dict set datp $peer $d
      }
    }
  }

  method getGameMode {} {
    return $currMode
  }

  method connectionLost {why} {
    log "Connection lost: $why"
  }
  ### END: NetIface functions
}
