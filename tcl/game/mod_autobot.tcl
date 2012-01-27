# Extends the updateThis to automatically add/remove AI when the
# player count is not the desired number.
# If the player count is below desired and we are the overseer,
# add a new AI bot.
# If there are more players than desired and we have AIs, remove
# the first one.
# This assumes that the extended connectVPeer method is available.
set AUTOBOT_AI_STD_PROB 0.5
class MixinAutobot {
  variable desired

  constructor {des} {
    set desired $des
  }

  # Returns the type of AI to use for new connections.
  # Default is randomly attachStdAi or attachGenAi (50%).
  protected method getAutobotType {} {
    if {rand() < $::AUTOBOT_AI_STD_PROB} {
      return attachStdAi
    } else {
      return attachGenAi
    }
  }

  # Checks for any new bots that should be added
  method autobotCheck {} {
    # Count total vpeers
    set count 0
    set peers [$this getPeers]
    if {$peers == {}} { set peers 0 }
    foreach peer $peers {
      incr count [llength [$this dpgp $peer list]]
    }

    if {$count > $desired} {
      foreach vpeer [$this dpg list] {
        # Bots' names begin with #
        if {"#" == [string index [$this dpg $vpeer name] 0]} {
          # Found
          $this disconnectVPeer $vpeer
          break
        }
      }
    } elseif {$count < $desired && [$this isOverseer]} {
      $this connectVPeer {} [$this getAutobotType]
    }
  }

  # Runs autobotCheck $desired times
  method autobotCheckAll {} {
    for {set i 0} {$i < $desired} {incr i} {
      autobotCheck
    }
  }

  method updateThis et {
    autobotCheck
    chain $et
  }
}
