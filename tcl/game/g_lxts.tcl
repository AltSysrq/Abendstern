class G_LXTS {
  inherit MixinAutobot MixinScoreFrags MixinScoreTeamfrag MixinTeam \
          MixinRound MixinMatch MixinRoundSpawn MixinStatsTeam \
          MixinPerfectRadar BasicGame

  constructor {fieldw fieldh background desiredPlayers nteams {initPeers {}}} {
    MixinAutobot::constructor $desiredPlayers
    MixinTeam::constructor $nteams
    BasicGame::constructor $fieldw $fieldh $background $initPeers
  } {
    if {[llength $initPeers] < 2} {
      initialiseTeams
    }
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
    format "[_ A game g_lxts_long] ([_ A game g_lxts]): $rnd" \
      $teamCount $teamCount
  }

  method getRoundIntermissionMainText {} {
    set survivor [dsg survivor]
    if {$survivor != {}} {
      return [format [_ A game survivor_team_fmt] $survivor]
    } else {
      return [_ A game survivor_nobody]
    }
  }

  method getRoundIntermissionSecondaryText {} {
    return {}
  }

  method isRoundRunning {} {
    set survivingTeam -1
    set numPlayers 0
    foreach peer [getPeers] {
      incr numPlayers [llength [dpgp $peer list]]
      foreach vp [dpgp $peer list] {
        if {![dpgp $peer $vp alive]} continue
        set team [dpgp $peer $vp team]
        if {$survivingTeam != -1 && $survivingTeam != $team} {
          # More than one team alive
          return yes
        }
        set survivingTeam $team
      }
    }

    # Round over, set survivor and give bonus
    # points to surviving team
    if {$survivingTeam == -1} {
      dss survivor {}
      return no
    } else {
      dss survivor [format "\a\[%s%s\a\]" \
        $::teamColourCode($survivingTeam) \
        $::teamName($survivingTeam)]

      set scores [dsg teamScores]
      set score [lindex $scores $survivingTeam]
      incr score [expr {max(1,$numPlayers/$teamCount)}]
      lset scores $survivingTeam $score
      dss teamScores $scores
      return no
    }
  }
}
