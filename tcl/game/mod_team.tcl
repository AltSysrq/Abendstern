# Adds general teams to the game.
# Up to six teams are supported:
#   #   Name    Colour          Long name
#   0   WDA     Red             Werstern Democratic Aliance
#   1   EVF     Blue            Europäche Vereignete Föderation
#   2   РКФ     Green           Российская Космическая Федерация (Russian Cosmic Federation)
#   3   CNSA    Yellow          China National Space Administration
#   4   ES      Magenta         Empira Solarza
#   5   LR      Cyan            Lunar Republic
#
# Ships and player names are coloured according to their team; a player's
# colour component is 0.6*team+0.4*player.
#
# This mixin does not implement a status panel.
#
# Ships friendly to the player are white instead of team-coloured.
#
# The getGenAiScore method is extended to add
#   (teamScore)-(maximumOtherTeamScore)
# to the result.
#
# Global dats:
#   teams       List of 6 bools, indicates which teams are present
#   teamScores  List of 6 ints, indicating team scores
# VPeer dats:
#   team        Which team the vpeer belongs to
# Overseer commands:
#   change-team vpeer team
#     Commands that the given vpeer be moved to the given team on the
#     next spawn.

set teamName(0) WUDR
set teamName(1) EVF
set teamName(2) РКФ
set teamName(3) CNSA
set teamName(4) ES
set teamName(5) LR

set teamColour(0,r) 1
set teamColour(0,g) 0
set teamColour(0,b) 0
set teamColour(1,r) 0
set teamColour(1,g) 0
set teamColour(1,b) 1
set teamColour(2,r) 0
set teamColour(2,g) 1
set teamColour(2,b) 0
set teamColour(3,r) 1
set teamColour(3,g) 1
set teamColour(3,b) 0
set teamColour(4,r) 1
set teamColour(4,g) 0
set teamColour(4,b) 1
set teamColour(5,r) 0
set teamColour(5,g) 1
set teamColour(5,b) 1

set teamColourCode(0) FF0000FF
set teamColourCode(1) 0000FFFF
set teamColourCode(2) 00FF00FF
set teamColourCode(3) FFFF00FF
set teamColourCode(4) FF00FFFF
set teamColourCode(5) 00FFFFFF

class MixinTeam {
  # Check every 5 seconds for an imbalance when we are overseer,
  # and transfer players as needed.
  variable timeUntilRebalance

  protected variable teamCount

  constructor nteams {
    set teamCount $nteams
    set timeUntilRebalance 5000
  }

  # Initialises team data.
  # This MUST be run before any vpeers connect.
  protected method initialiseTeams {} {
    set t {}
    for {set i 0} {$i < 6} {incr i} {
      lappend t [expr {$i < $teamCount}]
    }
    $this dss teams $t
    $this dss teamScores {0 0 0 0 0 0}
  }

  # Places the given vpeer into a random team
  method initVPeer vp {
    set ok no
    while {!$ok} {
      set team [expr {int(rand()*6)}]
      set ok [lindex [$this dsg teams] $team]
    }
    $this dps $vp team $team
    $this broadcastMessage format team_join \
                             [$this dpg $vp name] \
                             $::teamColourCode($team) \
                             $::teamName($team)
    chain $vp
  }

  # Sets the appropriate insignia on the given local ship.
  method postSpawn {vp ship} {
    $ship configure -insignia [expr {[$this dpg $vp team]+1}]
    chain $vp $ship
  }

  # Sets the altered colour on an incomming ship.
  # This chains first, so that it can override the colour
  # set by default.
  method modifyIncomming {peer vp ship} {
    chain $peer $vp $ship

    # If local, colour appropriately (don't change if human)
    if {$peer == 0} {
      if {$vp != 0} {
        $ship setColour {*}[getTeamColour yes 0 $vp]
      }
    } else {
      # If remote, assume it has its natural colour,
      # then fade it to the team's.
      # (This doesn't exactly hold for remote bots, which
      #  will be faded already, but this will just fade them
      #  more.)
      $ship setColour {*}[mixTeamColour yes \
                          [expr {[$ship cget -insignia]-1}] \
                          [$ship getColourR] [$ship getColourG] \
                          [$ship getColourB]]
    }
  }

  method getStatsColour {peer vp} {
    $this formatColour {*}[getTeamColour no $peer $vp]
  }

  private method mixTeamColour {enableMatch team r g b} {
    if {$team < 0 || $team > 5} {
      return [list $r $g $b]
    }

    set humanTeam -1
    catch {
      set humanTeam [$this dpg 0 team]
    }
    if {$enableMatch && $team == $humanTeam} {
      set tr 1
      set tg 1
      set tb 1
    } else {
      set tr $::teamColour($team,r)
      set tg $::teamColour($team,g)
      set tb $::teamColour($team,b)
    }

    list \
      [expr {0.4*$r + $tr*0.6}] \
      [expr {0.4*$g + $tg*0.6}] \
      [expr {0.4*$b + $tb*0.6}]
  }

  private method getTeamColour {enableMatch peer vp} {
    mixTeamColour $enableMatch \
                  [$this dpgp $peer $vp team] \
                  [$this dpgp $peer $vp colour r] \
                  [$this dpgp $peer $vp colour g] \
                  [$this dpgp $peer $vp colour b]
  }

  method resetScores {} {
    chain
    if {[$this isOverseer]} {
      $this dss teamScores {0 0 0 0 0 0}
    }
  }

  # Extend update to reassign teams when necessary
  method updateThis et {
    set timeUntilRebalance [expr {$timeUntilRebalance - $et}]
    if {[$this isOverseer] && $timeUntilRebalance < 0} {
      set timeUntilRebalance 5000

      # Count the team membership in cnt($team) and
      # accumulate members in members($team)
      for {set i 0} {$i < 6} {incr i} {
        set cnt($i) 0
        set members($i) {}
      }
      foreach peer [$this getPeers] {
        foreach vpeer [$this dpgp $peer list] {
          set t [$this dpgp $peer $vpeer team]
          if {$t >= 0 && $t < 6} {
            incr cnt($t)
            lappend members($t) [list $peer $vpeer]
          }
        }
      }

      # Find min and max
      set mincount 999
      set maxcount -1
      for {set i 0} {$i < 6} {incr i} {
        if {[lindex [$this dsg teams] $i]} {
          if {$cnt($i) < $mincount} {
            set mincount $cnt($i)
            set minteamix $i
          }
          if {$cnt($i) > $maxcount} {
            set maxcount $cnt($i)
            set maxteam $members($i)
          }
        }
      }

      # If there is more than a 1-player imbalance,
      # transfer one at random to the smaller team.
      if {$maxcount > $mincount+1} {
        lassign [lindex $maxteam [expr {int(rand()*[llength $maxteam])}]] mvp mvvp
        $this unicastMessage $mvp change-team $mvvp $minteamix
      }
    }

    chain $et
  }

  # Extend setting the match up to reset teams and fully balance
  # them (unless joining)
  method setupMatch join {
    chain $join
    if {[$this isOverseer] && !$join} {
      set t {}
      for {set i 0} {$i < 6} {incr i} {
        lappend t [expr {$i < $teamCount}]
      }
      $this dss teams $t

      # Gather peer/vpeer pairs
      set indices {}
      set pvp {}
      foreach peer [$this getPeers] {
        foreach vp [$this dpgp $peer list] {
          lappend pvp $peer $vp
          lappend indices [expr {[llength $indices]*2}]
        }
      }
      # Shuffle pairs
      set pairs {}
      while {[llength $indices]} {
        set ix [expr {int(rand()*[llength $indices])}]
        set index [lindex $indices $ix]
        set indices [lreplace $indices $ix $ix]
        lappend pairs {*}[lrange $pvp $index $index+1]
      }

      # Assign peers to teams
      set t 0
      foreach {peer vp} $pairs {
        $this unicastMessage $peer change-team $vp $t
        set t [expr {($t+1)%$teamCount}]
      }
    }
  }

  method receiveUnicast {from message} {
    set type [lindex $message 0]
    switch -exact -- $type {
      change-team {
        if {$from != [$this getOverseer]} { error "not from overseer" }
        lassign $message type vpeer team
        $this broadcastMessage format team_change \
                          [$this dpg $vpeer name] \
                          $::teamColourCode([$this dpg $vpeer team]) \
                          $::teamName([$this dpg $vpeer team]) \
                          $::teamColourCode($team) \
                          $::teamName($team)
        $this dps $vpeer team $team
        # Force respawn
        set ship [$this getOwnedShip $vpeer]
        if {$ship != 0} {
          $ship spontaneouslyDie
        }
        # If this could be the human, we need to recolour
        # all ships according to their insignias.
        # (This will again reduce the intensity of the custom
        #  colour, but making the team colours consistent is
        #  more important.)
        if {$vpeer == 0} {
          set field [$this cget -field]
          for {set i 0} {$i < [$field size]} {incr i} {
            set go [$field at $i]
            if {[$go getClassification] == "GOClassShip"} {
              $go setColour {*}[mixTeamColour yes \
                                [expr {[$go cget -insignia]-1}] \
                                [$go getColourR] \
                                [$go getColourG] \
                                [$go getColourB]]
            }
          }
        }
        return
      }
    }

    chain $from $message
  }

  method restrictSpectator spect {
    catch {
      $spect requireInsignia [expr {1+[$this dpg 0 team]}]
    }
  }

  method getIntermissionMainText {} {
    set maxScore -9999
    set teams [$this dsg teams]
    set teamScores [$this dsg teamScores]
    for {set t 0} {$t < 6} {incr t} {
      if {[lindex $teams $t] && [lindex $teamScores $t] > $maxScore} {
        set maxScore [lindex $teamScores $t]
        set winningTeam "\a\[$::teamColourCode($t)$::teamName($t)\a\]"
      }
    }

    format [_ A game winner_fmt] $winningTeam
  }

  method getIntermissionSecondaryText {} {
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
    format [_ A game best_player_fmt] $winner
  }

  method loadSchemata sec {
    chain $sec
    $this loadSchema mod_team $sec
  }
}
