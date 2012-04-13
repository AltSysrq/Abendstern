# Straight Deathmatch mode

class G_DM {
  inherit MixinAutobot MixinScoreFrags MixinMatch MixinFreeSpawn \
          MixinPerfectRadar MixinStatsFFA BasicGame

  constructor {desiredPlayers env comm} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $env $comm
  } {
    startOrJoinMatch
    initHuman
    if {[isOverseer]} {
      autobotCheckAll
    }
  }

  method getGameModeDescription {} {
    return "[_ A game g_dm_long] ([_ A game g_dm])"
  }
}
