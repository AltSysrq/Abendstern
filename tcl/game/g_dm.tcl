# Straight Deathmatch mode

class G_DM {
  inherit MixinAutobot MixinScoreFrags MixinMatch MixinFreeSpawn \
          MixinPerfectRadar MixinStatsFFA BasicGame

  constructor {fieldw fieldh background desiredPlayers {initPeers {}}} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $fieldw $fieldh $background $initPeers
  } {
    startOrJoinMatch
    initHuman
    autobotCheckAll
  }

  method getGameModeDescription {} {
    return "[_ A game g_dm_long] ([_ A game g_dm])"
  }
}
