# Default match time, in seconds.
# This should be altered by AI staging to effectively disable
# the match feature.
set DEFAULT_MATCH_TIME [expr {15*60}]

# This mixin divides gameplay into discrete matches (bounded
# by default by 15 minute intervals); at the end of a match,
# all Ships are killed and the match winner is shown; after
# a short intermission, scores are reset and a new match begins.
# Matches are a separate concept from rounds.
# Note that at the end of a match, all pending after scripts
# (BasicGame after, not ::after) are cancelled.
# This overrides the getRespawnTime from respawn managers to
# return zero at the beginning of a match; it must therefore
# be inherited before a respawn manager.
class MixinMatch {
  variable matchJustStarted no;# If true, respawn time is zero

  # Starts a new match, or joins an existing match.
  # This must be called manually after construction, but
  # before any vpeers are connected.
  method startOrJoinMatch {} {
    set startNew yes
    # If the matchRunning variable already exists, join
    # existing.
    catch {
      set running [$this dsg matchRunning]
      set startNew no
      if {$running} {
        # Join, without resetting (which would happen with
        # startMatch if we were overseer)
        set matchJustStarted yes
        setupMatch yes
        $this after 10 [code $this matchNotJustStarted]
        # No match initialisation should be performed, we're done
      } else {
        # Wait for start-match signal
        set im [new MatchIntermission $this]
        $this setMainMode $im
      }
    }

    if {$startNew} startMatch
  }

  # Starts a new match.
  method startMatch {} {
    if {[$this isOverseer]} {
      $this dss matchRunning yes
      $this broadcastMessage start-match
    }
  }

  method matchNotJustStarted {} {
    set matchJustStarted no
  }

  # Performs preparation for a match which must be done regardless
  # of match semantics (eg, team creation and assignment).
  # Default chains.
  # The argument is true if an already-running match is being joined.
  method setupMatch {joining} { chain $joining }

  # Performs initialisation of a match, specific to the match semantics.
  # Default sets 15 minute timer.
  method initMatch {} {
    if {[$this isOverseer]} {
      $this dss matchEndTime [expr {[::abnet::nclock]+$::DEFAULT_MATCH_TIME}]
    }
    chain
  }

  # Called when we are the overseer. Returns true if the current match
  # is still in progress.
  # Default returns whether time has not expired.
  method isMatchRunning {} {
    expr {[$this dsg matchEndTime] > [::abnet::nclock]}
  }

  method getRespawnTime {vp ship} {
    if {$matchJustStarted} {
      return 0
    } else {
      return [chain $vp $ship]
    }
  }

  method updateThis et {
    if {[$this isOverseer] && [$this dsg matchRunning]
    &&  ![isMatchRunning]} {
      endMatch
    } elseif {[$this isOverseer] && ![$this dsg matchRunning]
          &&  [$this dsg matchStartTime] <= [::abnet::nclock]} {
      startMatch
    }
    chain $et
  }

  # Terminates the current match.
  method endMatch {} {
    $this dss matchRunning no
    $this broadcastMessage end-match
    $this dss matchStartTime [expr {[::abnet::nclock]+15}]
  }

  method receiveBroadcast {peer msg} {
    lassign $msg type
    switch -exact -- $type {
      start-match {
        if {[$this getOverseer] != $peer} { error "not overseer" }
        $this clearAfters
        [cget -field] clear
        $this resetScores
        set matchJustStarted yes
        $this after 10 [code $this matchNotJustStarted]
        $this setMainMode [new BasicGameMode $this {}]
        setupMatch no
        initMatch

        return
      }

      end-match {
        if {[$this getOverseer] != $peer} { error "not overseer" }

        # Change to intermission
        set im [new MatchIntermission $this]

        # Kill all ships
        foreach vp [$this dpg list] {
          set ship [$this getOwnedShip $vp]
          if {$ship != 0} { $ship spontaneouslyDie }
        }
        $this clearAfters

        # Don't set the mode until after killing ships, since
        # we need to replace the respawn screen
        # (But we need to create the mode while the Ship is
        #  still valid.)
        [cget -env] setReference [$im getSpectator] false
        $this setMainMode $im

        return
      }
    }
    chain $peer $msg
  }

  # Returns the primary text to show during intermission.
  # Default is [format [_ A game winner_fmt] $winner], where
  # $winner is the name of the player with the highest score.
  method getIntermissionMainText {} {
    set maxScore -9999
    foreach peer [$this getPeers] {
      foreach vp [$this dpgp $peer list] {
        if {$maxScore < [$this dpgp $peer $vp score]} {
          set maxScore [$this dpgp $peer $vp score]
          set winner \
"\a\[[$this getStatsColour $peer $vp][$this dpgp $peer $vp name]\a\]"
        }
      }
    }
    format [_ A game winner_fmt] $winner
  }

  # Returns the secondary text to show during intermission.
  # Default is an empty string.
  method getIntermissionSecondaryText {} {
    return ""
  }

  # Returns the ship owned by vpeer zero, or 0 otherwise
  method getIntermissionHumanShip {} {
    $this getOwnedShip 0
  }

  # Returns true if the beginning of a match starts
  # an actual game (ie, whether to spawn at the beginning
  # of a match).
  # Default returns true.
  method matchStartsGame {} { return yes }

  method loadSchemata sec {
    chain $sec
    $this loadSchema mod_match $sec
  }
}

# Extends the SpectatorMode to show a larger message at the centre
# of the screen.
# This can also be used for round intermissions (replace type ""
# with "Round" in constructor)
class MatchIntermission {
  inherit SpectatorMode

  variable match
  variable mainText
  variable subText
  variable itype

  # Constructs the intermission for the given match.
  # If a type is specified, it is placed between
  # "get" and "Intermission" in method calls
  # other than getIntermissionHumanShip.
  constructor {m {type {}}} {
    SpectatorMode::constructor [$m cget -env] [$m cget -field] \
      [$m getIntermissionHumanShip] true "$this getMessage"
  } {
    set match $m
    set mainText [$m "get${type}IntermissionMainText"]
    set subText [$m "get${type}IntermissionSecondaryText"]
    set itype $type
  }

  method getMessage {} {
    if {$itype == {}} {
      format [_ A game newmatch_in_fmt] \
        [expr {[$match dsg matchStartTime]-[::abnet::nclock]}]
    } else {
      format [_ A game newround_in_fmt] \
        [expr {[$match dsg roundStartTime]-[::abnet::nclock]}]
    }
  }

  method draw {} {
    chain
    set f $::gui::font
    ::gui::colourStd
    # Draw main text (2x scale, centred)
    glPushMatrix
    # Centre
    glTranslate 0.5 [expr {$::vheight/2.0}]
    # Rescale
    glUScale 2.0
    glPushMatrix
    # Move from centre to bottom left of string
    glTranslate [expr {-0.5*[$f width $mainText]}] \
                [expr {-0.5*[$f getHeight]}]
    $f preDraw
    $f drawStr $mainText 0 0
    $f postDraw
    glPopMatrix ;# Restore to scaled

    # Move down a line
    glTranslate 0 [expr {-1.0*[$f getHeight]}]
    # Reduce size (to 1.5x)
    glUScale 0.75
    # Move to bottom left of string
    glTranslate [expr {-0.5*[$f width $subText]}] \
                [expr {-0.5*[$f getHeight]}]
    $f preDraw
    $f drawStr $subText 0 0
    $f postDraw

    # Restore matrix
    glPopMatrix
  }
}
