# This mixin facilitates dividing gameplay into rounds, where
# a set number of rounds comprise a match.
# This mixin does not handle spawning, since some round-based
# modes will still use free spawning (such as HVC) while others
# use round-delimited spawning (eg, LMS and LXTS).
#
# The mixin assumes the presence of MixinMatch and overrides some
# of its functions, so it must be inherited before MixinMatch.
#
# Subclasses must provide an
#   isRoundRunning
# method, which returns true if the current round is still in
# progress. Additionally:
#   getRoundIntermissionMainText
#   getRoundIntermissionSecondaryText
set ROUNDS_IN_MATCH 10
class MixinRound {
  variable justStartedRound no

  method roundNotJustStarted {} {
    set justStartedRound no
  }

  method isMatchRunning {} {
    expr {[$this dsg currentRound] < $::ROUNDS_IN_MATCH}
  }

  method isMatchTimed {} {
    return no
  }

  method getRespawnTime {vp ship} {
    if {$justStartedRound} {
      return 0
    } else {
      return [chain $vp $ship]
    }
  }

  method initMatch {} {
    if {[$this isOverseer]} {
      # Start a new round
      $this dss currentRound 0
      $this dss roundRunning yes
      $this broadcastMessage start-round
    }
    chain
  }

  method setupMatch join {
    chain $join
    # If joining, we won't recvive the start-round signal
    if {$join && [$this dsg roundRunning]} {
      setupRound yes
    }
  }

  method updateThis et {
    if {[$this isOverseer] && [$this dsg roundRunning]
    &&  ![$this isRoundRunning]} {
      $this dss roundRunning no
      # Start next round in 15 seconds
      $this dss roundStartTime [expr {[::abnet::nclock]+7}]
      $this broadcastMessage end-round
    }

    if {[$this isOverseer]
    &&  ![$this dsg roundRunning]
    &&  [$this dsg matchRunning]
    &&  [::abnet::nclock] > [$this dsg roundStartTime]} {
      # Time for the next round (don't start a round if
      # this will be the end of the match)
      set n [$this dsg currentRound]
      incr n
      $this dss currentRound $n
      if {$n < $::ROUNDS_IN_MATCH} {
        $this dss roundRunning yes
        $this broadcastMessage start-round
      }
    }

    chain $et
  }

  method receiveBroadcast {peer msg} {
    lassign $msg type
    switch -exact -- $type {
      start-round {
        if {$peer != [$this getOverseer]} { error "not overseer" }
        [cget -field] clear
        setupRound no
        return
      }
      end-round {
        if {$peer != [$this getOverseer]} { error "not overseer" }
        endRound
        return
      }
    }
    chain $peer $msg
  }

  # Performs any beginning-of-round operations.
  # Default sets spawn time to zero temporarily and sets the
  # mode to normal.
  # The argument is true if we are joining an already-existing round.
  method setupRound join {
    set justStartedRound yes
    $this after 10 $this roundNotJustStarted
    $this setMainMode [new BasicGameMode $this {}]
    chain $join
  }

  # Performs any end-of-round operations.
  # Default switches to intermission, kills all ships, and
  # cancels all after scripts.
  method endRound {} {
    set im [new MatchIntermission $this Round]
    foreach vp [$this dpg list] {
      set ship [$this getOwnedShip $vp]
      if {$ship != 0} { $ship spontaneouslyDie }
    }
    $this clearAfters
    chain
    # Set this last so that a respawn manager's mode switch
    # is overridden
    $this setMainMode $im
    [cget -env] setReference [$im getSpectator] false
  }

  method matchStartsGame {} { return no }

  method loadSchemata sec {
    chain $sec
    $this loadSchema mod_round $sec
  }
}
