set spawnOverrideColourByType no
# Contains data and methods shared between the various spawn managers.
class SpawnManager {
  protected variable vpeerMethods {} ;# Dict mapping vpeers to controller methods
  protected variable vpeerShips {} ;# Dict mapping vpeers to ships to use
  protected variable oldHumanShip 0 ;# Used for SpecatorMode
  protected variable humanRespawnTime ;# To be able to show a countdown to the user
  protected variable isDisconnectingVPeer no ;# If true, don't do anything in listener
  protected variable resetOnSpectator yes ;# Reset the first time, then never again
  protected variable pendingSpawn {} ;# If a vpeer is in this dict, spawn is pending

  method clearAfters {} {
    chain
    # Clear the pending list, since there is no longer an after
    # script waiting
    set pendingSpawn {}
  }

  # Connects a new vpeer.
  # If the controller method is omitted, it is assumed to
  # be attachHuman.
  method connectVPeer {{name {}} {control attachHuman}} {
    set vp [chain $name]
    if {$::spawnOverrideColourByType} {
      if {$control == "attachHuman"} {
        $this dps $vp colour {r 0 g 1 b 0}
      } elseif {$control == "attachStdAi"} {
        $this dps $vp colour {r 1 g 0 b 0}
      } elseif {$control == "attachGenAi"} {
        $this dps $vp colour {r 0 g 0 b 1}
      }
    }
    dict set vpeerMethods $vp $control
    if {"attachHuman" != $control} {
      selectRandomVPeerShip $vp
    } else {
      selectHumanVPShip $vp
    }
    return $vp
  }

  method selectHumanVPShip vp {
    # Use conf.preferred.main if it exists and is of the appropriate class;
    # otherwise use conf.preferred.$class
    set class [$this cget -gameClass]
    set ok no
    catch {
      set ok [expr {$class eq [$ str [$ str conf.preferred.main].info.class]}]
      if {$ok} {
        setVPeerShip $vp [$ str conf.preferred.main]
      }
    }
    if {!$ok} {
      setVPeerShip $vp [$ str conf.preferred.$class]
    }
  }

  method disconnectVPeer vp {
    set isDisconnectingVPeer yes
    dict unset vpeerMethods $vp
    dict unset vpeerShips $vp
    dict unset pendingSpawn $vp
    chain $vp
    set isDisconnectingVPeer no
  }

  # Randomly selects a ship for the given vpeer from hangar.effective.
  method selectRandomVPeerShip vp {
    set ix [expr {int(rand()*[$ length hangar.effective])}]
    dict set vpeerShips $vp [$ str hangar.effective.\[$ix\]]
  }

  # Explicitly sets a particular shipMount for a vpeer.
  method setVPeerShip {vp ship} {
    dict set vpeerShips $vp $ship
  }

  # Spawns a ship.
  # If spawning fails, randomly selects a new ship and tries again.
  # On success with humans, enters BasicGameMode and sets oldHumanShip.
  method spawnDoSpawn vp {
    # It is possible that the vpeer was disconnected since we
    # decided to spawn something for it
    # See if it still exists
    if {![dict exists $vpeerMethods $vp]} return

    set method [dict get $vpeerMethods $vp]
    set shipMount [dict get $vpeerShips $vp]

    set ship [$this spawn $vp $shipMount $method [expr {$method == "attachHuman"}]]
    if {0 == $ship} {
      # Not successful, so use a different ship.
      selectRandomVPeerShip $vp
      spawnDoSpawn $vp
      return
    }

    if {$method == "attachHuman"} {
      # Done spectating
      $this setMainMode [new BasicGameMode $this {}]
      set oldHumanShip $ship
    }

    $this addShipDeathListener $ship spawnListener
  }

  # If teams are enabled, restricts the Spectator to the appropriate team.
  # Default does nothing.
  method restrictSpectator spect {
  }
}
