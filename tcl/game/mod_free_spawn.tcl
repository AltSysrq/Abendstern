# This mixin is a respawn manager in which spawning is controlled
# only by a possible delay.
# It also handles setting a SpectatorMode during the inter-spawn
# time.
#
# It replaces connectVPeer with a version that takes an additional
# argument (after the name) which is the method to use to attach
# a controller to the vpeer's ships.
#
# This class must be inherited before BasicGame, so that its methods
# override BasicGame's.
#
# Humans' ship is set to $::humanShipMount, which MUST be set already.
#
# If ::spawnOverrideColourByType is set to true, vpeer colours
# are overriden by control type as follows:
#   human       0,1,0
#   stdai       1,0,0
#   genai       0,0,1

class MixinFreeSpawn {
  inherit SpawnManager

  # Keep track of vpeers for which we currently have a timer.
  # This is needed in case a vpeer is disconnected and then reconnected while
  # there is still a timer on it; it will get stuck in an endless respawn loop.
  # When a timer is added, set this; do not add a timer if set. When the timer
  # fires, unset that vpeer regardless of what happens next.
  # This is a dict (set) keyed on the vpeer.
  variable spawnPendingTimers {}

  method connectVPeer args {
    set vp [chain {*}$args]
    initRespawn $vp
    return $vp
  }

  # Prepares to spawn a ship for the given vpeer.
  # If the peer is human, will use setMainMode to
  # use a SpectatorMode.
  private method initRespawn vp {
    if {[dict exists $spawnPendingTimers $vp]} return
    if {[dict exists $pendingSpawn $vp]} return ;# Already pending

    set method [dict get $vpeerMethods $vp]
    if {"attachHuman" == $method} {
      set spectatorMode [new SpectatorMode [cget -env] [cget -field] \
                         $oldHumanShip yes [code $this getSpectatorTime]]
      set oldHumanShip 0
      $this setMainMode $spectatorMode
      restrictSpectator [$spectatorMode getSpectator]
      set resetOnSpectator no
    }

    dict set spawnPendingTimers $vp {}
    set delay [getRespawnTime $vp [dict get $vpeerShips $vp]]
    $this after $delay $this spawnTrigger $vp
    dict set $pendingSpawn $vp {}

    if {"attachHuman" == $method} {
      set humanRespawnTime [expr {[[cget -field] cget -fieldClock]+$delay}]
    }
  }

  method spawnTrigger vp {
    dict unset spawnPendingTimers $vp
    $this spawnDoSpawn $vp
  }

  # Called when a ship dies.
  method spawnListener {ship del vp} {
    if {!$isDisconnectingVPeer} {
      initRespawn $vp
    }
  }

  # Returns the number of milliseconds to wait before spawning
  # the specified vpeer/ship
  # Default returns 3 seconds
  method getRespawnTime {vp ship} {
    return 15000
  }

  private method getSpectatorTime {} {
    set seconds [expr {($humanRespawnTime - [[cget -field] cget -fieldClock])/1000}]
    format [_ A game respawn_in_fmt] $seconds
  }

  # Extend setupMatch to begin spawning
  method setupMatch join {
    chain $join
    if {[$this matchStartsGame]} {
      foreach vp [$this dpg list] {
        initRespawn $vp
      }
    }
  }

  # Same for setupRound
  method setupRound join {
    chain $join
    foreach vp [$this dpg list] {
      initRespawn $vp
    }
  }
}
