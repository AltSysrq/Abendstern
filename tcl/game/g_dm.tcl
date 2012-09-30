# Straight Deathmatch mode

class G_DM {
  inherit MixinAutobot MixinScoreFrags MixinMatch MixinFreeSpawn \
          MixinPerfectRadar MixinStatsFFA \
          MixinSAWClock MixinSAWBestPlayer MixinSAWLocalPlayer \
          MixinSAWMatchTimeLeft \
          BasicGame

  constructor {desiredPlayers env comm class} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $env $comm $class
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
