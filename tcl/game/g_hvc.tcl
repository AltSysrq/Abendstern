class G_HVC {
  inherit MixinPerfectRadar MixinAutobot MixinRound MixinMatch \
          MixinFreeSpawn MixinScoreFrags MixinStatsFFA \
          MixinSAWBestPlayer MixinSAWLocalPlayer MixinSAWRoundsOfMatch \
          MixinSAWClock \
          BasicGame

  # Humans are team 1 (neutral with each other) and
  # cyborgs are team 2 (friendly with each other).
  # Cyborgs are given a "team" entry so that they count
  # as teamkills if they hurt each other, while humans
  # are not assigned a team via peer data.

  # There are a couple of possible cases to end up with
  # zero cyborgs. In such a case, assimilate a new player
  # and don't check this condition until this hits zero again.
  variable timeUntilZeroCyborgCheck

  constructor {desiredPlayers env comm class} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $env $comm $class
  } {
    startOrJoinMatch
    initHuman
    if {[isOverseer]} {
      autobotCheckAll
    }
    setAlliance ANeutral 1 1
    if {[isOverseer]} {
      dss survivor {}
    }

    # If we are the only peer, any created vpeers are not assimilated
    if {1 == [llength [getPeers]]} {
      foreach vp [dpg list] {
        unassimilate $vp
      }
    }
  }

  destructor {
    clear_insignias
  }

  method initVPeer vp {
    chain $vp
    dps $vp assimilated 1
    dps $vp team 2
  }

  method getGameModeDescription {} {
    if {[dsg currentRound] < $::ROUNDS_IN_MATCH} {
      set rnd [format [_ A game round_fmt] \
                  [expr {[dsg currentRound]+1}] $::ROUNDS_IN_MATCH]
    } else {
      set rnd [_ A game match_over]
    }
    return "[_ A game g_hvc_long] ([_ A game g_hvc]): $rnd"
  }

  method unassimilate {vp} {
    dps $vp assimilated 0
    catch {
      set dp [dpg $vp]
      dict unset dp team
      dps $vp $dp
    }
  }

  method setupRound join {
    if {!$join} {
      foreach vp [dpg list] {
        unassimilate $vp
      }
    }

    set timeUntilZeroCyborgCheck 2048
    if {[isOverseer]} {
      dss nextAssimilationIsFirst 1
    }
    chain $join
  }

  method updateThis et {
    set timeUntilZeroCyborgCheck [expr {$timeUntilZeroCyborgCheck-$et}]
    if {$timeUntilZeroCyborgCheck < 0 && [isOverseer]} {
      set numCyborgs 0
      set numPlayers 0
      set pairs {}
      foreach peer [getPeers] {
        incr numPlayers [llength [dpgp $peer list]]
        foreach vp [dpgp $peer list] {
          lappend pairs [list $peer $vp]
          if {[dpgp $peer $vp assimilated]} { incr numCyborgs }
        }
      }

      if {$numCyborgs == 0} {
        if {[dsg nextAssimilationIsFirst]} {
          set cnt [expr {max(1,$numPlayers/4)}]
        } else {
          set cnt 1
        }
        dss nextAssimilationIsFirst 0
        for {set i 0} {$i < $cnt} {incr i} {
          set pair [lindex $pairs [expr {int(rand()*[llength $pairs])}]]
          unicastMessage [lindex $pair 0] assimilate [lindex $pair 1]
        }
      }

      set timeUntilZeroCyborgCheck 2048
    }
    chain $et
  }

  method postSpawn {vp ship} {
    chain $vp $ship
    if {[dpg $vp assimilated]} {
      $ship configure -insignia 2
    } else {
      $ship configure -insignia 1
    }
  }

  method shipKilledBy {ship lvp peer rvp} {
    # Assimilate if killed by cyborg or suicide
    if {![dpg $lvp assimilated] &&
        ([dpgp $peer $rvp assimilated] || ($peer == 0 && $lvp == $rvp))} {
      $this after 2048 $this assimilate $lvp
    }
  }

  method receiveUnicast {peer msg} {
    lassign $msg type
    switch -exact -- $type {
      assimilate {
        assimilate [lindex $msg 1]
        return
      }
    }
    chain $peer $msg
  }

  method assimilate vp {
    dps $vp assimilated 1
    dps $vp team 2 ;# For teamkills
    broadcastMessage format hvc_assimilated_fmt \
      "\a\[[getStatsColour 0 $vp][dpg $vp name]\a\]"
    set ship [getOwnedShip $vp]
    if {$ship != 0} {
      $ship configure -insignia 2
    }
  }

  method isRoundRunning {} {
    set survivor {}
    foreach peer [getPeers] {
      foreach vp [dpgp $peer list] {
        if {![dpgp $peer $vp assimilated]} {
          # If found a second survivor, round not yet done
          if {$survivor != {}} { return yes }

          set survivor \
          "\a\[[getStatsColour $peer $vp][dpgp $peer $vp name]\a\]"
        }
      }
    }

    # Game over, set survivor and we're done
    dss survivor $survivor
    return no
  }

  method endRound {} {
    # Count players
    set numPlayers 0
    foreach peer [getPeers] {
      incr numPlayers [llength [dpgp $peer list]]
    }
    # Add points to any survivor
    foreach vp [dpg list] {
      if {![dpg $vp assimilated]} {
        set score [dpg $vp score]
        incr score $numPlayers
        incr score $numPlayers
        dps $vp score $score
      }
    }
    chain
  }

  method getStatusAreaElements {} {
    set l [chain]
    set humans 0
    set cyborgs 0
    foreach peer [getPeers] {
      foreach vp [dpgp $peer list] {
        if {[dpgp $peer $vp assimilated]} {
          incr cyborgs
        } else {
          incr humans
        }
      }
    }

    lappend l 2 [format [_ A game hvc_humans_cyborgs_cnt_fmt] $humans $cyborgs]
    return $l
  }

  method getRoundIntermissionMainText {} {
    set s [dsg survivor]
    if {$s == {}} {
      return [_ A game survivor_nobody]
    } else {
      return [format [_ A game survivor_fmt] $s]
    }
  }

  method getRoundIntermissionSecondaryText {} {
    if {[dsg survivor] == {}} {
      return [_ A game hvc_resistance_was_futile]
    } else {
      return {}
    }
  }

  method loadSchemata sec {
    chain $sec
    loadSchema g_hvc $sec
  }
}
