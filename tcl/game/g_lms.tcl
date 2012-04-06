class G_LMS {
  inherit MixinPerfectRadar MixinAutobot MixinScoreFrags \
          MixinRound MixinMatch MixinRoundSpawn MixinStatsFFA \
          BasicGame

  constructor {desiredPlayers env comm} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $env $comm
  } {
    initHuman
    autobotCheckAll
    startOrJoinMatch
    if {[isOverseer]} {dss survivor {}}
  }

  method getGameModeDescription {} {
    if {[dsg currentRound] < $::ROUNDS_IN_MATCH} {
      set rnd [format [_ A game round_fmt] \
                  [expr {[dsg currentRound]+1}] $::ROUNDS_IN_MATCH]
    } else {
      set rnd [_ A game match_over]
    }
    return "[_ A game g_lms_long] ([_ A game g_lms]): $rnd"
  }

  method getRoundIntermissionMainText {} {
    set survivor [dsg survivor]
    if {$survivor != {}} {
      return [format [_ A game survivor_fmt] $survivor]
    } else {
      return [_ A game survivor_nobody]
    }
  }

  method getRoundIntermissionSecondaryText {} {
    return ""
  }

  method endRound {} {
    # Count players
    set numPlayers 0
    foreach peer [getPeers] {
      incr numPlayers [llength [dpgp $peer list]]
    }
    # Give bonus points to anyone of ours that survived
    foreach vp [dpg list] {
      if {[dpg $vp alive]} {
        set score [dpg $vp score]
        incr score $numPlayers
        incr score $numPlayers
        dps $vp score $score
      }
    }
    chain
  }

  method isRoundRunning {} {
    set cnt 0
    foreach peer [getPeers] {
      foreach vp [dpgp $peer list] {
        if {[dpgp $peer $vp alive]} {
          incr cnt
          set survivor \
            "\a\[[getStatsColour $peer $vp][dpgp $peer $vp name]\a\]"
        }
      }
    }
    if {$cnt == 0} {
      dss survivor {}
      return false
    } elseif {$cnt == 1} {
      dss survivor $survivor
      return false
    } else {
      return true
    }
  }

  method loadSchemata sec {
    chain $sec
    loadSchema g_lms $sec
  }
}
