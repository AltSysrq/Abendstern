class G_XTDM {
  inherit MixinAutobot MixinScoreFrags MixinScoreTeamfrag MixinTeam \
          MixinStatsTeam MixinPerfectRadar MixinMatch MixinFreeSpawn \
          BasicGame

  constructor {desiredPlayers nteams env comm} {
    MixinAutobot::constructor $desiredPlayers
    MixinTeam::constructor $nteams
    BasicGame::constructor $env $comm
  } {
    if {[isOverseer]} {
      initialiseTeams
    }
    startOrJoinMatch
    initHuman
    if {[isOverseer]} {
      autobotCheckAll
    }
  }

  method getGameModeDescription {} {
    format  "[_ A game g_xtdm_long] ([_ A game g_xtdm])" $teamCount $teamCount
  }
}