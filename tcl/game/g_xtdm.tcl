class G_XTDM {
  inherit MixinAutobot MixinScoreFrags MixinScoreTeamfrag MixinTeam \
          MixinStatsTeam MixinPerfectRadar MixinMatch MixinFreeSpawn \
          BasicGame

  constructor {fieldw fieldh background desiredPlayers nteams {initpeers {}}} {
    MixinAutobot::constructor $desiredPlayers
    MixinTeam::constructor $nteams
    BasicGame::constructor $fieldw $fieldh $background $initpeers
  } {
    if {[llength $initpeers] < 2} {
      initialiseTeams
    }
    startOrJoinMatch
    initHuman
    autobotCheckAll
  }

  method getGameModeDescription {} {
    format  "[_ A game g_xtdm_long] ([_ A game g_xtdm])" $teamCount $teamCount
  }
}