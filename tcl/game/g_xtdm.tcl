class G_XTDM {
  inherit MixinAutobot MixinScoreFrags MixinScoreTeamfrag MixinTeam \
          MixinStatsTeam MixinPerfectRadar MixinMatch MixinFreeSpawn \
          MixinSAWBestPlayer MixinSAWLocalPlayer MixinSAWBestTeam \
          MixinSAWLocalTeam MixinSAWMatchTimeLeft MixinSAWClock \
          BasicGame

  constructor {desiredPlayers nteams env comm class} {
    MixinAutobot::constructor $desiredPlayers
    MixinTeam::constructor $nteams
    BasicGame::constructor $env $comm $class
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