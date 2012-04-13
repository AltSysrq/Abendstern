# This mixin is a spawn manager that only permits spawning
# at the beginning of rounds.
class MixinRoundSpawn {
  inherit SpawnManager

  method setupRound join {
    chain $join
    if {$join} {
      # Can't spawn in middle of round
      roundSpawnUseSpectator
    } else {
      # Spawn all vpeers
      foreach vp [$this dpg list] {
        spawnDoSpawn $vp
      }
    }
  }

  method spawnListener {ship del vp} {
    # The dict entry will not exist if this is due to a vpeer disconnecting.
    if {[dict exists $vpeerMethods $vp]} {
      if {"attachHuman" == [dict get $vpeerMethods $vp]} {
        roundSpawnUseSpectator
      }
    }
  }

  method roundSpawnUseSpectator {} {
    set s [new SpectatorMode [cget -env] [cget -field] $oldHumanShip true \
              [code _ A game waiting_for_next_round]]
    set oldHumanShip 0
    $this setMainMode $s
    restrictSpectator [$s getSpectator]
    set resetOnSpectator no
  }
}
