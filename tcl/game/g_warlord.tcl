class G_Warlord {
  inherit MixinPerfectRadar MixinAutobot MixinRound MixinMatch \
          MixinFreeSpawn \
          MixinSAWBestPlayer MixinSAWLocalPlayer MixinSAWRoundsOfMatch \
          MixinSAWClock \
          BasicGame

  # Each vpeer stores the insignia it will use (to avoid collisions) and a list
  # of superiors it has, in externalised {peer vpeer} format. The first item in
  # the list is its direct superior; the others are indirect superiors, used to
  # repair the tree when players disappear.
  #
  # Cycles are ignored, and resolved locally in an undefined manner. If a local
  # vpeer becomes part of a cycle (which can happen with properly-functioning
  # clients if two players kill each other), the relation is deleted.
  #
  # Messages:
  # unicast:
  #   release-to {externalised-pvp ... } local-vp
  #     The local vpeer specified is now subordinate to the parent list
  #     specified
  #   teamkill-release-subordinates local-vp
  #     The given local vpeer must release its subordinates due to having
  #     comitted a teamkill.

  # The tree of subordination at the time of last calculation. This is a dict
  # whose keys are internalised {peer vpeer} pairs, and whose value is a list
  # of dicts with the same structure as this one, indicating further
  # subordinate parts of the tree.
  variable subordinateTree {}

  # The time until the next recalculation of the subordinate tree
  variable timeUntilSubordinateTreeRecalc 1000

  constructor {desiredPlayers env comm class} {
    MixinAutobot::constructor $desiredPlayers
    BasicGame::constructor $env $comm $class
  } {
    startOrJoinMatch
    initHuman
    if {[isOverseer]} {
      autobotCheckAll
      dss warlord {}
    }
  }

  destructor {
    clear_insignias
  }

  method initVPeer vp {
    chain $vp

    # TODO: We need to handle people jumping into the middle of a round
    # correctly. Currently, this allows them to get top-level status. Perhaps
    # they should be given to another player at random when joining in the
    # middle of a round?
    dps $vp superiors {}
    dps $vp teamkillKeepProb 1.0

    set ok no
    while {!$ok} {
      set insig [expr {1 + int(0x7FFFFFFE * rand())}]
      set ok yes

      foreach peer [getPeers] {
        foreach v [dpgp $peer list] {
          catch {
            if {$insig == [dpgp $peer $v insignia]} {
              set ok no
            }
          }
          if {!$ok} break
        }
        if {!$ok} break
      }
    }

    dps $vp insignia $insig
  }

  method getGameModeDescription {} {
    if {[dsg currentRound] < $::ROUNDS_IN_MATCH} {
      set rnd [format [_ A game round_fmt] \
                  [expr {[dsg currentRound]+1}] $::ROUNDS_IN_MATCH]
    } else {
      set rnd [_ A game match_over]
    }
    return "[_ A game g_wlrd_long] ([_ A game g_wlrd]): $rnd"
  }

  private method resetWarlordVPData {} {
    foreach vp [dpg list] {
      dps $vp superiors {}
      dps $vp teamkillKeepProb 1.0
    }
  }

  # Returns the internalised {peer vpeer} direct superior to the given
  # peer/vpeer, or the empty string if there is none.
  # The returned combination is guaranteed to exist.
  private method getDirectSuperior {peer vp} {
    set sups [dpgp $peer $vp superiors]
    set ix 0
    foreach sup $sups {
      set sup [internalise-pvp $sup]
      # Ignore if the referenced peer does not exist
      if {{} eq [lindex $sup 0]} continue

      # See if the vpeer exists
      if {-1 != [lsearch -exact [dpgp [lindex $sup 0] list] [lindex $sup 1]]} {
        # If this is a local vpeer and the index is not zero, delete the broken
        # links
        if {$peer == 0 && $ix != 0} {
          dps $vp superiors [lrange $sups $ix end]
        }
        return $sup
      }

      incr ix
    }

    # If this is a local vpeer, clear the superiors list since it is broken
    if {$peer == 0 && [llength $sups]} {
      dps $vp superiors {}
    }
    return {}
  }

  # Helper for recalcSubordinateTree
  private method scanTree {k subs subords occurredVar} {
    upvar $occurredVar occurred
    set accum {}
    dict set occurred $k {}

    foreach sub $subs {
      if {[dict exists $subordinateTree $sub]} {
        # Not actually a root, move it into accum
        dict set accum $sub [dict get $subordinateTree $sub]
        dict unset subordinateTree $sub
      } elseif {![dict exists $occurred $sub]} {
        dict set accum $sub [scanTree $sub [dict get $subords $sub] \
                                 $subords occurred]
      }
    }

    return $accum
  }

  # Sets insignias for all peers in the given tree.
  # parents is a list of parent insignias.
  # siblings is a list of insignias that are in the tree, but are not
  # necessarily parents.
  # Returns an updated siblings list.
  private method setInsigniaTree {subs parents siblings} {
    dict for {pvp subsubs} $subs {
      set insig [dpgp {*}$pvp insignia]

      # All siblings are neutral
      foreach sib $siblings {
        setAlliance ANeutral $insig $sib
      }
      # Parents are friendly (override what was set in the above loop)
      foreach par $parents {
        setAlliance AAllies $insig $par
      }

      lappend siblings $insig
      set subParents $parents
      lappend subParents $insig
      set siblings [setInsigniaTree $subsubs $subParents $siblings]
    }

    return $siblings
  }

  # Examines the subordinate relationships to reconstruct the tree and to set
  # insignia relationships.
  # Local vpeers' superiors data are also updated to reflect the real tree
  # structure.
  method recalcSubordinateTree {} {
    # First, produce a subordinate mapping
    set subords {}
    foreach peer [getPeers] {
      foreach vp [dpgp $peer list] {
        set sup [getDirectSuperior $peer $vp]
        if {$sup ne {}} {
          dict lappend subords $sup [list $peer $vp]
        }

        if {![dict exists $subords [list $peer $vp]]} {
          dict set subords [list $peer $vp] {}
        }
      }
    }

    # Construct the tree. For each subordinate relation where the key has not
    # yet been scanned, assume that the key is a root. For each item it
    # references: If it is a root, move it into a subordinate; if it has not
    # yet occurred, construct that branch now; otherwise, ignore (cross edge).
    set occurred {}
    set subordinateTree {}
    dict for {k subs} $subords {
      if {[dict exists $occurred $k]} continue

      # OK, assume it is a root
      # Don't put into the tree proper until we're done processing, so direct
      # loops are ignored
      dict set subordinateTree $k [scanTree $k $subs $subords occurred]
    }

    # Reset all insignias -- This takes care of all enemy relations
    clear_insignias

    dict for {root subs} $subordinateTree {
      setInsigniaTree $subs [list [dpgp {*}$root insignia]] {}
    }

    # Rewrite superior relations
    foreach vp [dpg list] {
      set sups {}
      set occurred {}
      set fst [getDirectSuperior 0 $vp]
      for {set curr $fst} {$curr ne {}} \
          {set curr [getDirectSuperior {*}$curr]} {
        if {[dict exists $occurred $curr]} break
        dict set occurred $curr {}
        lappend sups $curr
      }

      if {$sups ne [dpg $vp superiors]} {
        dps $vp superiors $sups
      }
    }
  }

  method setupRound join {
    if {!$join} {
      resetWarlordVPData
    }
    recalcSubordinateTree

    chain $join
  }

  method updateThis et {
    set timeUntilSubordinateTreeRecalc \
        [expr {$timeUntilSubordinateTreeRecalc-$et}]
    if {$timeUntilSubordinateTreeRecalc < 0} {
      set timeUntilSubordinateTreeRecalc 1024
      recalcSubordinateTree
    }

    chain $et
  }

  method postSpawn {vp ship} {
    chain $vp $ship
    $ship configure -insignia [dpg $vp insignia]
  }

  method shipKilledBy {ship lvp peer rvp} {
    set killedInsignia [dpg $lvp insignia]
    set killerInsignia [dpgp $peer $rvp insignia]

    if {"AEnemies" eq [getAlliance $killedInsignia $killerInsignia]} {
      # Our ship was killed by an enemy; it now belongs to him, and our
      # direct subordinates are released to him
      becomeSubordinateTo $lvp $peer $rvp
      releaseSubordinates $lvp
    } elseif {"AAllies" eq [getAlliance $killedInsignia $killerInsignia]} {
      # Our ship was killed by a friend (or this was a suicide)!
      # Tell them they need to release their subordinates
      unicastMessage $peer teamkill-release-subordinates $rvp
    }
  }

  private method becomeSubordinateTo {lvp peer rvp} {
    set supsup [dpgp $peer $rvp superiors]
    dps $lvp superiors [list [externalise-pvp [list $peer $rvp]] {*}$supsup]
  }

  private method releaseSubordinates {vp {keepProb 0.0}} {
    set sups [dpg $vp superiors]
    foreach peer [getPeers] {
      foreach rvp [dpgp $peer list] {
        if {[list 0 $vp] eq [getDirectSuperior $peer $rvp] &&
            rand() > $keepProb} {
          unicastMessage $peer release-to $sups $rvp
        }
      }
    }
  }

  method receiveUnicast {peer msg} {
    lassign $msg type
    if {$type eq "release-to"} {
      lassign $msg type sups vp

      # Ensure that sups is sane
      if {![string is list $sups]} return

      foreach sup $sups {
        if {![string is list -strict $sup]} return
        if {2 != [llength $sup]} return
        foreach s $sup {
          if {![string is integer -strict $s]} return
        }
      }

      # OK
      dps $vp superiors $sups
    } elseif {$type eq "teamkill-release-subordinates"} {
      lassign $msg type vp
      if {$vp ni [dpg list]} return

      releaseSubordinates $vp [dpg $vp teamkillKeepProb]
      dps $vp teamkillKeepProb [expr {[dpg $vp teamkillKeepProb]/2.0}]
    } else {
      chain $peer $msg
    }
  }

  method isTeamKill {killer lvp} {
    set killerInsignia [dpgp {*}$killer insignia]
    set killedInsignia [dpg $lvp insignia]

    expr {"AAllies" eq [getAlliance $killerInsignia $killedInsignia]}
  }

  method isRoundRunning {} {
    if {[dict size $subordinateTree] == 1} {
      # Down to one player, he has won
      set winner [lindex [dict keys $subordinateTree] 0]

      lassign $winner peer vp

      dss warlord \
        "\a\[[getStatsColour $peer $vp][dpgp $peer $vp name]\a\]"
      return no
    } else {
      return yes
    }
  }

  method endRound {} {
    recalcSubordinateTree
    awardScores $subordinateTree
    chain
  }

  private method awardScores {roots} {
    set count [dict size $roots]
    dict for {root subs} $roots {
      set icount [awardScores $subs]
      if {[lindex $root 0] == 0} {
        # Local vpeer, award points
        set vp [lindex $root 1]
        set score [dpg $vp score]
        set score [expr {$score + 4*[dict size $roots] + $icount}]
        dps $vp score $score
      }
      incr count $icount
    }

    return $count
  }

  method getRoundIntermissionMainText {} {
    set s [dsg warlord]
    return [format [_ A game warlord_fmt] $s]
  }

  method getRoundIntermissionSecondaryText {} {
    return ""
  }

  private method depthFirstList {roots depth} {
    set accum {}
    dict for {k subs} $roots {
      lappend accum $depth $k
      set accum [concat $accum [depthFirstList $subs [expr {$depth+1}]]]
    }

    return $accum
  }

  method getStatsPanel {} {
    set barColourRotation {FF00FF FF0000 00FFFF FFFF00 0000FF 00FF00}
    set stats {}
    foreach {depth pvp} [depthFirstList $subordinateTree 0] {
      lassign $pvp peer
      set name [dpgp {*}$pvp name]
      set score [dpgp {*}$pvp score]
      set colour [getStatsColour {*}$pvp]
      if {$peer == [getOverseer] &&
          [lindex $pvp 1] == [lindex [dpgp $peer list] 0]} {
        set overseer "\a{\a\[(white)!\a\]\a}"
      } else {
        set overseer {}
      }

      if {[dpgp {*}$pvp alive]} {
        set alive "\a\[(special)+\a\]"
      } else {
        set alive "\a\[(danger)-\a\]"
      }

      if {$depth == 0} {
        # Next bar colour
        set fst [lindex $barColourRotation 0]
        set barColourRotation  [lrange $barColourRotation 1 end]
        lappend barColourRotation $fst
      }

      set bars [string repeat "| " $depth]
      set barColour [lindex $barColourRotation 0]
      set text [format "\a\[%sFF%s\a\]%s\a\[%s%s\a\] %s" \
                    $barColour $bars $alive $colour $name $overseer]
      set elt [new gui::BorderContainer]
      $elt setElt left [new ::gui::Label $text left]
      $elt setElt right [new ::gui::Label $score right]
      lappend stats $elt
    }

    set ncolumns [expr {min(3,max(1,([llength $stats]+6)/12))}]

    set top [new ::gui::BorderContainer 0 0.01]
    set main [new ::gui::HorizontalContainer 0.02 grid]
    set ix 0
    for {set col 0} {$col < $ncolumns} {incr col} {
      set percol [expr {[llength $stats]/$ncolumns}]
      if {$col < [llength $stats]%$ncolumns} {
        # Distribute the remainder over the left-most columns
        incr percol
      }

      set vpan [new ::gui::VerticalContainer 0.002]
      for {set row 0} {$row < $percol} {incr row; incr ix} {
        $vpan add [lindex $stats $ix]
      }

      #$main add [new ::gui::Frame $vpan]
      $main add $vpan
    }

    $top setElt centre $main
    set timeLeftNote {}
    catch {
      set endTime [$this dsg matchEndTime]
      set timeLeft [expr {$endTime-[::abnet::nclock]}]
      if {$timeLeft >= 0 && [$this isMatchTimed]} {
        set timeLeftNote \
          [format ": [_ A game time_left_fmt]" \
            [expr {$timeLeft/60}] [expr {$timeLeft%60}]]
      }
    }
    $top setElt top \
      [new ::gui::Label "[$this getGameModeDescription]$timeLeftNote" centre]
    $top setElt bottom [new ::gui::Label [_ A game stats_overseer_note] left]

    return $top
  }

  method loadSchemata sec {
    chain $sec
    loadSchema g_warlord $sec
  }
}
