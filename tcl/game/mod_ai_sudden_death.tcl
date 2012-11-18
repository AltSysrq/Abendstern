# Causes all living ships to receive a damage multiplier of
# $::SUDDEN_DEATH_DAMAGE_MULT when only vpeers whose names begin with '#' remain
set SUDDEN_DEATH_DAMAGE_MULT 10
class MixinAiSuddenDeath {
  variable hasSuddenDeathBeenEnabled no

  method setupMatch {joining} {
    set hasSuddenDeathBeenEnabled no
    chain $joining
  }

  method setupRound {joining} {
    set hasSuddenDeathBeenEnabled no
    chain $joining
  }

  method aiSuddenDeathCheck {} {
    if {[$this isOverseer] && !$hasSuddenDeathBeenEnabled} {
      set anyHumans no
      set anyAlive no
      foreach peer [$this getPeers] {
        foreach vp [$this dpgp $peer list] {
          set anyAlive [expr {$anyAlive || [$this dpgp $peer $vp alive]}]
          if {"#" ne [string index [$this dpgp $peer $vp name] 0] &&
              [$this dpgp $peer $vp alive]} {
            set anyHumans yes
            break
          }
        }
        if {$anyHumans} break
      }

      if {$anyAlive && !$anyHumans} {
        $this broadcastMessage ai-sudden-death
        $this broadcastMessage format entering_sudden_death
      }
    }
  }

  method updateThis et {
    aiSuddenDeathCheck
    chain $et
  }

  method receiveBroadcast {peer msg} {
    lassign $msg type
    if {$type == "ai-sudden-death"} {
      if {$peer != [$this getOverseer]} {
        return ;# Not from overseer, ignore
      }

      set hasSuddenDeathBeenEnabled yes

      foreach vp [$this dpg list] {
        set ship [$this getOwnedShip $vp]
        if {$ship != 0} {
          $ship configure -damageMultiplier $::SUDDEN_DEATH_DAMAGE_MULT
        }
      }
    } else {
      chain $peer $msg
    }
  }
}
