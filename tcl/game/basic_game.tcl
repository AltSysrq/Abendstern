# The BasicGame class is an Application which abstracts most of the networking
# concerns and provides the most common functionality shared among the various
# game modes (other than STRA).
#
# The network INFO system into an ad-hoc, temporary, pseudo-server-client
# model of information distribution. From among the peers is selected the
# Overseer, which is always the peer with the lowest userid and has indicated
# it is ready to perform this function.
#
# For high-level information, the Overseer plays the role of a server. The
# Overseer determines the true value of all game variables and the outcome of
# votes (this trust is safe since we only let official, unmodified copies of
# Abendstern join games).
#
# Both normal peers and the Overseer maintain all game state (both for
# convenience and so that nothing is lost if the current Overseer disappears).
# The Underlings send the Overseer semantic information about change in game
# state, while the Overseer broadcasts the new values of game variables.
#
# Any peer will respond as an Overseer when queried as such, even if it knows
# it is an Underling. This allows the network to be more resilient.
#
# This class by itself cannot do much as far as actual gameplay or data logic.
# Subclasses must determine spawning rules, provide an Overseer interface, and
# handle data events themselves.
#
# Any Modes run under this Application must have an enableRawInputForwarding,
# which determines whether input is sent to the HumanController or the GUI.
# If they return 2, they will both have raw input forwarding and standard events.
# They MAY have a backgroundUpdate method, which is called every frame with the
# elapsed time, even if a secondary mode is currently primary.
#
# Damage blame is based on a 16-bit number, the upper byte representing a number
# assigned to the peer (0 = local) and the lower corresponding to a vpeer.
class BasicGame {
  inherit ::gui::Application

  # The GameEnv used in this game.
  # This pointer should not be modified.
  public variable env
  # Value returned by [$env cget -field]
  public variable field

  # List of list{peer,userid}s.
  # Sorted according to ascending userid
  # Local "peer" is NULL (0).
  variable peers
  # The external representation of this peer
  public variable thisPeer

  # Maps constant numbers to peers.
  variable peersByNumber ;# num->peer
  variable peersByNumberRev ;# peer->num

  # Dict of shared data (ie, data not specific to any peer)
  protected variable dats
  # Dict of per-peer data (first level is userid of peer).
  # The second level consists of "virtual peers" (numeric)
  # and list, which lists entries under virtual peers
  # to be considered.
  # It is safe to assume that if any vpeer is a human, it
  # is vpeer zero.
  protected variable datp
  # When acting as True Overseer, a list of dict paths within
  # dats that have changed since the last broadcast of deltata.
  variable datsDeltata
  # Similar to datsDeltata except applying to datp.
  variable datpDeltata

  # Stack of available vpeer numbers (0..255)
  variable emptyVPeers

  # Whether networking is enabled.
  variable networkingEnabled

  # The time in milliseconds until we next query the Overseer
  # about data changes
  variable timeUntilQuery

  # Dict mapping ship*s to code fragments to eval on death (with the
  # virtual peer number and ship* appended)
  variable shipDeathListeners
  # fun<void:Ship*,bool> that maps to our callback
  variable shipDeathFun
  # Dict mapping ship*s to virtual peer indices
  variable shipOwnership
  # Reverse of shipOwnership
  variable shipOwnershipRev

  # The CommonKeyboardClient subclass that forwards appropriate events
  # to us
  variable ckc

  # The current HumanController, or 0 if none exists
  variable human

  # Whether to pause the game
  variable paused
  # When the "pause" menu is open, this contains the Mode to switch
  # back to when closed.
  # Empty string otherwise.
  # Subclasses should not affect mode directly, therefore; instead,
  # use setMainMode.
  variable unpauseMode

  # Debugging options
  variable fast
  variable slow
  variable frameXframe
  variable timeUntilFXFAdvance

  # Value to return from updateThis
  variable retval

  # List of {fieldClock,script} pairs, sorted by fieldClock,
  # of after scripts
  variable afters

  # Tracks the single kill streaks for local vpeers.
  # Maps vpeer->int
  # Increment on enemy kill, decrement on teamkill, zero on death.
  # Maintained via kill-notification unicast messages.
  variable sks
  # Tracks the inter-player kill streaks against local vpeers.
  # Maps peerA->vpeerA->vpeerB->int
  # Increment when A kills B, zero when B kills A.
  variable iks

  # Used by the GeneticAI methods to store the score of the
  # AI when it was created (this is subtracted from the final score)
  variable genaiBaseScores

  # Stores the most recent field clock of submitting genetic AI
  # scoring information.
  variable lastGenAiReportFC

  # Constructs a BasicGame.
  # fieldw      Width of field
  # fieldh      Height of field
  # background  Script to evaluate to the background to use; $field is the game field.
  #             Defaults to an empty StarField.
  # peers       Initial peers; empty list disables networking
  constructor {fieldw fieldh {background {new StarField default 0 $field 1}} {initpeers {}}} {
    ::gui::Application::constructor
  } {
    set env [new GameEnv default $fieldw $fieldh]
    set field [$env getField]
    $env configure -stars [eval $background]
    set peers $initpeers
    set thisPeer 0
    set networkingEnabled [expr {0 != [llength $peers]}]
    set shipDeathFun [new BasicGameShipDeathCallback $this]
    set ckc [new BasicGameCKC $this]
    set lastGenAiReportFC [$field cget -fieldClock]

    set peersByNumber {}
    set peersByNumberRev {}
    set i 0
    foreach peer [getPeers] {
      dict set peersByNumber $i $peer
      dict set peersByNumberRev $peer $i
      incr i
    }

    set dats {}
    set datp {}
    dict set datp 0 list {}
    set emptyVPeers {}
    for {set i 0} {$i < 256} {incr i} {lappend emptyVPeers $i}
    set datsDeltata {}
    set timeUntilQuery 0
    set shipDeathListeners {}
    set shipOwnership {}
    set shipOwnershipRev {}
    set afters {}

    set human 0
    attachHuman {} 0

    set paused no
    set unpauseMode {}
    set fast no
    set slow no
    set frameXframe no
    set timeUntilFXFAdvance 0
    set retval 0

    set sks {}
    set iks {}
    set genaiBaseScores {}

    set mode [new BasicGameMode $this {}]

    ship_mixer_init

    # Load schemata
    ::schema::init
    loadSchemata p
    ::schema::define validateDatp
    ::schema::init
    loadSchemata s
    ::schema::define validateDats
  }

  destructor {
    delete object $env
    delete object $shipDeathFun
    delete object $ckc

    ship_mixer_end
  }

  method initHuman {} {
    set humanvp [connectVPeer $::abnet::username]
    dps $humanvp colour r [$ float conf.ship_colour.\[0\]]
    dps $humanvp colour g [$ float conf.ship_colour.\[1\]]
    dps $humanvp colour b [$ float conf.ship_colour.\[2\]]
  }

  # BEGIN: BASIC MANAGEMENT AND FORWARDING
  method setMode m {
    lappend ::gui::autodelete $mode
    set mode $m
  }

  # Sets the primary mode appropriately, regardless of the
  # current pause state.
  method setMainMode m {
    if {{} == $unpauseMode} {
      setMode $m
    } else {
      delete object $unpauseMode
      set unpauseMode $m
    }
  }

  method finish {{rv {}}} {
    if {$rv == {}} { set rv $this }
    set retval $rv
  }

  method updateThis et {
    $mode update $et
    catch {$mode backgroundUpdate $et }
    if {$unpauseMode != {}} {
      catch {$unpauseMode backgroundUpdate $et }
    }
    if {!$paused} {
      if {$fast} {set et [expr {$et*10.0}]}
      if {$slow} {set et [expr {$et/10.0}]}
      if {$frameXframe} {
        set timeUntilFXFAdvance [expr {$timeUntilFXFAdvance-$et}]
        set et 0
        if {$timeUntilFXFAdvance <= 0} {
          set timeUntilFXFAdvance [expr {$timeUntilFXFAdvance+256}]
          set et 50
        }
      }
      if {$et > 0} {
        $env update $et
      }

      # Run any needed after scripts
      while {[llength $afters] &&
             [lindex $afters 0 0] < [$field cget -fieldClock]} {
        namespace eval :: [lindex $afters 0 1]
        set afters [lrange $afters 1 end]
      }
    }

    set timeUntilQuery [expr {$timeUntilQuery-$et}]
    if {$timeUntilQuery < 0} {
      set timeUntilQuery [expr {1024+1024*rand()}]
      queryOverseer
    }
    if {[llength $datpDeltata]} {
      broadcastDatp
    }
    return $retval
  }

  method drawThis {} {
    $env draw
    # Go back to standard coordinates
    configureGL
  }

  # Like ::after, but based on field time.
  method after {delay args} {
    lappend afters [list [expr {[$field cget -fieldClock]+$delay}] $args]
    set afters [lsort -integer -index 0 $afters]
  }

  # Clears the afters list
  method clearAfters {} {
    set afters {}
  }

  # For mixin access.
  # Will never return an empty list
  method getPeers {} { if {{} != $peers} { return $peers } else { return 0 } }

  # END: BASIC MANAGEMENT AND FORWARDING

  # BEGIN: DATA ACCESS AND MODIFICATION
  # Loads any schemata associated with the class.
  # The argument indicates either s or p (for dats and datp, respectively).
  # This may be called before the object is fully constructed, so mixins
  # cannot rely on any of their constructors having been called.
  #
  # chain should be called first, so that the schemata are loaded left-to-right
  # (in the inherit list).
  method loadSchemata sec {
    chain $sec
    loadSchema basic_game $sec
  }

  # Loads the schema with the given basename and section.
  method loadSchema {base sec} {
    ::schema::addFile tcl/game/$base.$sec.schema
  }

  # Performs a dict lookup within dats and returns the result.
  method dsg {args} {
    dict get $dats {*}$args
  }

  # Performs a dict lookup within {datp 0} and returns the result.
  # (ie, it returns the data for this peer)
  method dpg {args} {
    dict get $datp 0 {*}$args
  }

  # Performs a dict lookup within datp and returns the result.
  method dpgp {args} {
    dict get $datp {*}$args
  }

  # Alters the given shared data.
  # This should only be done by overseer methods.
  # The path will be added to all peers' deltata.
  method dss {args} {
    set path [lrange $args 0 end-1]
    set value [lindex $args end]
    dict set dats {*}$path $value
    lappend datsDeltata $path
  }

  # Alters the given local data in {datp 0}
  # The path will be added to the deltata
  method dps {args} {
    set path [lrange $args 0 end-1]
    set value [lindex $args end]
    dict set datp 0 {*}$path $value
    lappend datpDeltata $path
  }

  # Resets all scores. If a mixin defines scores beyond
  # individuals, it must extend this to reset those as well.
  method resetScores {} {
    foreach vp [dpg list] {
      dps $vp score 0
    }
  }

  # END: DATA ACCESS AND MODIFICATION

  # BEGIN: VIRTUAL PEER MANAGEMENT
  # Connects a new virtual peer of the given name.
  # If this is not the sole human player, the name should
  # be prefixed with #.
  # If no name is specified, one is autogenerated.
  method connectVPeer {{name {}}} {
    if {{} == $name} {
      set ok no
      while {!$ok} {
        set name "#[namegenAny]"
        set ok yes
        foreach vp [dpg list] {
          if {$name == [dpg $vp name]} {
            set ok no
            break
          }
        }
      }
    }

    set l [dpg list]
    set vp [lindex $emptyVPeers 0]
    set emptyVPeers [lrange $emptyVPeers 1 end]
    lappend l $vp
    incr nextVPeer
    dps list $l
    dps $vp name $name
    initVPeer $vp

    broadcastMessage format vpeer_connected_fmt [getStatsColour 0 $vp] $name

    return $vp
  }

  # Initialises any data needed by the new vpeer.
  # Default sets the colours randomly, and sets
  # score to 0.
  method initVPeer vp {
    dps $vp colour r [expr {rand()}]
    dps $vp colour g [expr {rand()}]
    dps $vp colour b [expr {rand()}]
    dps $vp score 0
    dps $vp alive 0
  }

  # Deletes the given vpeer.
  method disconnectVPeer vp {
    if {[dict exists $shipOwnership $vp]} {
      # Kill its ship
      [dict get $shipOwnership $vp] spontaneouslyDie
    }

    broadcastMessage format vpeer_disconnected_fmt \
      [getStatsColour 0 $vp] [dpg $vp name]

    dict set datp 0 [dict remove [dict get $datp 0] $vp]
    dps list [lsearch -exact -not -all -inline [dpg list] $vp]
    lappend emptyVPeers $vp
  }

  # Formats the given colour tripple into a hexadecimal code
  # for font drawing.
  method formatColour {r g b} {
    set r [expr {int(max(0, min($r*255, 255)))}]
    set g [expr {int(max(0, min($g*255, 255)))}]
    set b [expr {int(max(0, min($b*255, 255)))}]
    format %02X%02X%02XFF $r $g $b
  }

  # Returns the colour (as a hex code) for the given peer/vpeer combo.
  # Default returns the preferred colour of the vpeer (each component
  # minimised to 0.3)
  method getStatsColour {peer vpeer} {
    formatColour [expr {max(0.3, [dpgp $peer $vpeer colour r])}] \
                 [expr {max(0.3, [dpgp $peer $vpeer colour g])}] \
                 [expr {max(0.3, [dpgp $peer $vpeer colour b])}]
  }

  # END: VIRTUAL PEER MANAGEMENT

  # BEGIN: SHIPS AND CONTROLLERS
  # Spawns a new ship for the given vpeer, loaded from the specified
  # mount. The ccallback is a method that takes the vpeer and ship
  # and attaches a controller to it (this is done after the ship*->vpeer
  # dict has been updated).
  # After this, the postSpawn and modifyIncomming methods are called
  # on the ship.
  # Finally, the ship is returned after being added to the field.
  #
  # If an error occurs while loading, 0 is returned.
  #
  # If the given vpeer already has a ship, it is killed (this will
  # trigger callbacks!).
  #
  # If reference is true, the new ship will be set as the environment's
  # reference (with reset). When the ship is killed, the reference is
  # nullified.
  method spawn {vp mount ccallback {reference no}} {
    log "spawn $vp $mount $ccallback $reference"
    if {[dict exists $shipOwnership $vp]} {
      set old [dict get $shipOwnership $vp]
      $old spontaneouslyDie
      # Don't have to deregister, this is done in the callback
    }
    if {[catch {
      set ship [loadShip $field $mount]
    } err]} {
      log "Couldn't load ship $mount: $err"
      return 0
    }

    dict set shipOwnership $vp $ship
    dict set shipOwnershipRev $ship $vp
    dps $vp alive 1

    $this $ccallback $vp $ship
    postSpawn $vp $ship
    modifyIncomming 0 $vp $ship

    $ship configure -shipExistenceFailure [$shipDeathFun get]

    $field add $ship
    if {$reference} {
      $env setReference $ship yes
      set oldEH [$ship cget -effects]
      $ship configure -effects [$env cget -cam]
      delete object $oldEH
      $ship enableSoundEffects

      addShipDeathListener $ship nullifyReference
    }
    addShipDeathListener $ship shipDeathKillfeed
    return $ship
  }

  method nullifyReference {vp del ship} {
    $env setReference 0 no
  }

  # Perform post-creation modification of a locally-spawned ship.
  # Default sets the tag and blame.
  method postSpawn {vpeer ship} {
    $ship cget -tag
    $ship configure -tag "[dpg $vpeer name] ([$ str [$ship cget -typeName].info.name])"
    $ship configure -blame $vpeer
  }

  # Perorm post-reception modification of a locally- or remotely-spawned
  # ship. This is called after postSpawn for local ships.
  # Default sets the colour to the vpeer's.
  method modifyIncomming {peer vpeer ship} {
    if {$peer == 0} {
      $ship setColour [dpg $vpeer colour r] \
                      [dpg $vpeer colour g] \
                      [dpg $vpeer colour b]
    }
    # Else trust the remote colour
  }

  # Attaches a HumanController to the specified ship, which may be 0.
  # This will delete any currently-existing HumanController.
  # THIS WILL CAUSE SERIOUS ISSUES IF THE SHIP CONTAINING THIS HUMANCONTROLLER
  # IS STILL ACTIVE!
  method attachHuman {vp ship} {
    if {$human != "0"} {
      delete object $human
    }
    set human [new HumanController default $ship]
    hc_conf_clear
    $human hc_conf_bind
    $ckc hc_conf_bind
    [$env cget -cam] hc_conf_bind
    hc_conf_configure $human [$ str conf.control_scheme]

    if {$ship != "0"} {
      $ship configure -controller $human
      # Set difficulty
      if {!$networkingEnabled} {
        $ship configure -damageMultiplier [$ float conf.game.dmgmul]
      }
    }
  }

  # Attaches an AIControl for ai.test to the given ship.
  method attachStdAi {vp ship} {
    $ship configure -controller \
      [new AIControl default $ship ai.test {}]
  }

  # Attaches a GeneticAI to the given Ship.
  # If it fails, it falls back on attachStdAi
  method attachGenAi {vp ship} {
    if {[catch {
      set ai [GenAI_make $ship]
      if {$ai == 0} {error "AI constructor failed"}
      $ship configure -controller $ai

      dict set genaiBaseScores $vp [getGenAiScore $vp $ship]
      # We need to have this run LAST --- attach it on the next frame
      after 0 $this addShipDeathListener $ship submitGenAiScore
    } err]} {
      log "Couldn't attach GeneticAI; falling back on StdAI: $err"
      attachStdAi $vp $ship
    }
  }

  # Returns the score of the GeneticAI for the given
  # vp/ship combo. Default returns the Ship's score.
  method getGenAiScore {vp ship} {
    $ship cget -score
  }

  # Submits the score (as returned by getGenAiScore)
  # for the given GeneticAI (if connected)
  method submitGenAiScore {ship del vp} {
    if {[$ship cget -diedSpontaneously]} return
    set ai [$ship cget -controller]
    ::abnet::submitAiReport \
      [$ai cget -species] [$ai cget -generation] \
      [$ai getScores] \
      [expr {[$field cget -fieldClock]-$lastGenAiReportFC}]
    set lastGenAiReportFC [$field cget -fieldClock]
  }

  # Called by shipDeathFun.
  method shipDeath {ship deletion} {
    if {[$ship cget -effects] == $this} debugTclExports
    # This may be the environment camera; in this case,
    # this statement has no effect other than to remove
    # Tcl references to that object
    delete object [$ship cget -effects]
    $ship configure -shipExistenceFailure 0

    # If the ship had a HumanController, delete it and
    # use a shipless HumanController
    if {$human == [$ship cget -controller]} {
      set human 0 ;# Just let the Ship delete it
      attachHuman {} 0
    }

    set vp [getShipOwner $ship]

    # Call listeners
    if {[dict exists $shipDeathListeners $ship]} {
      foreach listener [dict get $shipDeathListeners $ship] {
        $this $listener $ship $deletion $vp
      }
      dict unset shipDeathListeners $ship
    }

    # Remove from vpeer
    if {{} != $vp} {
      dps $vp alive 0
      dict unset shipOwnership $vp
      dict unset shipOwnershipRev $ship
    }
  }

  # Adds the specified method to the listeners for the given ship's death.
  # These methods must take the following arguments:
  #   Ship*     The ship that died
  #   deletion  If true, the ship is about to be deleted
  #   vpeer     If non-empty, the local vpeer that owned the ship
  method addShipDeathListener {ship method} {
    dict lappend shipDeathListeners $ship $method
  }

  # Returns the vpeer that owns the given ship, or {} otherwise.
  method getShipOwner ship {
    if {[dict exists $shipOwnershipRev $ship]} {
      return [dict get $shipOwnershipRev $ship]
    } else {
      return {}
    }
  }

  # Returns the ship owned by the vpeer, or 0 if none
  method getOwnedShip vp {
    if {[dict exists $shipOwnership $vp]} {
      return [dict get $shipOwnership $vp]
    } else {
      return 0
    }
  }

  # Called when a local ship is killed by the given peer/vpeer.
  # Default does nothing.
  method shipKilledBy {ship lvp peer rvp} {}

  # Implements the killfeed.
  # The default should be satisfactory for most purposes,
  # understanding teams as well.
  # It will also call shipDeathAwardPoints appropriately.
  # Some messages are handled via the kill-notification unicast.
  # Messages and conditions:
  # self    *               suicide             (below)
  # team    *               teamkill            (below)
  # other   inter-ks <  3   kill                (below)
  # other   2<inter-ks<5    kill_again          (below)
  # other   inter-ks == 5   kill_dominate       (below)
  # other   inter-ks > 5    kill_domcont        (below)
  # other   rev-inter-ks>5  kill_revenge        (notification)
  # other   self-ks == 5    killing_spree       (notification)
  method shipDeathKillfeed {ship del vp} {
    dict set sks $vp 0
    if {[$ship cget -diedSpontaneously]} return
    set assist {}
    set secondary0 {}
    set secondary1 {}
    lassign [$ship getDeathAttributions] killer assist secondary0 secondary1
    set killer [decodeBlame $killer]
    set assist [decodeBlame $assist]
    set secondary0 [decodeBlame $secondary0]
    set secondary1 [decodeBlame $secondary1]

    # Don't show assists if the assister is the one who got killed
    if {{} == $assist || (0 == [lindex $assist 0] && $vp == [lindex $assist 1])} {
      set aex {}
      set fargs [list "\a\[[getStatsColour {*}$killer][dpgp {*}$killer name]\a\]" \
                      "\a\[[getStatsColour 0 $vp][dpg $vp name]\a\]"]
    } else {
      set aex _assist
      set fargs [list "\a\[[getStatsColour {*}$killer][dpgp {*}$killer name]\a\]" \
                      "\a\[[getStatsColour {*}$assist][dpgp {*}$assist name]\a\]" \
                      "\a\[[getStatsColour 0 $vp][dpg $vp name]\a\]"]
    }
    set which [expr {int(rand()*5)}]

    if {$killer != {}} {
      shipKilledBy $ship $vp {*}$killer

      # Check current kill-streak and possible teams for message type
      if {0 == [lindex $killer 0] && $vp == [lindex $killer 1]} {
        set who self
      } else {
        set who other
        catch {
          if {[dpgp {*}$killer team] == [dpg $vp team]} {
            set who team
          }
        }
      }

      if {$who == "self"} {
        broadcastMessage format kill_suicide$aex$which {*}$fargs
      } elseif {$who == "other"} {
        if {[dict exists $iks {*}$killer $vp]} {
          set n [dict get $iks {*}$killer $vp]
        } else {
          set n 0
        }
        incr n
        dict set iks {*}$killer $vp $n

        if {$n < 3} {
          broadcastMessage format kill_kill$aex$which {*}$fargs
        } elseif {$n < 5} {
          broadcastMessage format kill_kill_again$aex$which {*}$fargs
        } elseif {$n == 5} {
          broadcastMessage format kill_dominate$aex$which {*}$fargs
        } else {
          broadcastMessage format kill_domcont$aex$which {*}$fargs
        }
      } else {
        broadcastMessage format kill_teamkill$aex$which {*}$fargs
      }

      # Send necessary messages
      overseerMessage kill-notification $vp $killer $assist $secondary0 $secondary1
      foreach var {killer assist secondary0 secondary1} {
        if {{} != [set $var]} {
          unicastMessage [lindex [set $var] 0] \
            kill-notification $var [lindex [set $var] 1] $vp
        }
      }
    }
  }

  # Decodes the given blame integer into a peer,vpeer combination.
  method decodeBlame blame {
    if {$blame == {}} return {}
    set peern [expr {$blame >> 8}]
    set vpeer [expr {$blame & 0xFF}]
    if {[dict exists $peersByNumber $peern]} {
      set peer [dict get $peersByNumber $peern]
      # If the vpeer doesn't exist, give to first vpeer
      if {-1 == [lsearch -exact [dpgp $peer list] $vpeer]} {
        log "Warning: Unable to interpret blame: $blame (missing vpeer $vpeer)"
        set vpeer [lindex [dpgp $peer list] 0]
      }
      return [list $peer $vpeer]
    } else {
      return {}
    }
  }

  # END: SHIPS AND CONTROLLERS

  # BEGIN: OVERSEER AND MESSAGING INTERFACE

  # Returns the peer that is the overseer.
  # 0 indicates local.
  method getOverseer {} {
    if {$networkingEnabled} {
      error "Networking not yet supported for getOverseer"
    }

    return 0
  }

  # Returns true if we are the overseer.
  method isOverseer {} {
    expr {0 == [getOverseer]}
  }

  # Broadcasts the given high-level message to all peers.
  # This will also pass the message to receiveBroadcast.
  method broadcastMessage args {
    if {$networkingEnabled} {
      error "Networking not yet supported for broadcastMessage"
    }

    receiveBroadcast 0 $args
  }

  # Called when a broadcast message is received from the given peer.
  # Peer is 0 for local.
  # Default understands:
  #   format l10n args
  #     Evaluates to
  #       global_chat_post_local [format [_ A game $l10n] {*}$args]
  # Otherwise logs a warning that the message is not understood.
  # Overriders should always chain the method call on unknown messages.
  method receiveBroadcast {peer msg} {
    lassign $msg type
    if {$type == "format"} {
      set args [lassign $msg type l10n]
      global_chat_post_local [format [_ A game $l10n] {*}$args]
      return
    }
    log "Warning: Unknown broadcast received from $peer: $msg"
  }

  # Sends the given high-level message to the given peer.
  # If peer is 0, calls receiveUnicast.
  method unicastMessage {dst args} {
    if {$dst != 0} {
      error "Networking not yet supported for unicastMessage"
    }

    receiveUnicast 0 $args
  }

  # Called when a unicast message is received from the given peer.
  # Default understands:
  #   kill-notification {killer|assist|secondary0|secondary1} local-vpeer remote-vpeer
  #     iks and sks updated appropriately, and any needed notifications
  # Otherwise logs a warning that the message is not understood.
  # Overriders should always chain the method call on unknown messages.
  method receiveUnicast {peer msg} {
    lassign $msg type
    if {$type == "kill-notification"} {
      lassign $msg type killtype localvp remotevp
      if {-1 != [lsearch -exact [dpg list] $localvp]} {
        set increment 0
        if {$killtype == "killer"} {
          set increment 1
        }
        if {$peer == 0 && $localvp == $remotevp} {
          # Suicide (counter reset in death listener)
          set increment 0
        }
        catch {
          if {[dpg $localvp team] == [dpgp $peer $remotevp team]} {
            set increment -1
            dict set sks $localvp 0
          }
        }
        dict incr sks $localvp $increment
        if {[dict get $sks $localvp] == 5 && $increment > 0} {
          broadcastMessage format killing_spree \
            "\a\[[getStatsColour 0 $localvp][dpg $localvp name]\a\]"
        }

        if {-1 != [lsearch -exact [dpgp $peer list] $remotevp]
        &&  $increment > 0} {
          if {[dict exists $iks $peer $remotevp $localvp]
          &&  [dict get $iks $peer $remotevp $localvp] >= 5} {
            broadcastMessage format "kill_revenge[expr {int(rand()*5)}]" \
              "\a\[[getStatsColour 0 $localvp][dpg $localvp name]\a\]" \
              "\a\[[getStatsColour $peer $remotevp][dpgp $peer $remotevp name]\a\]"
          }
          dict set iks $peer $remotevp $localvp 0
        }
      }

      return
    }

    log "Warning: Unknown unicast received from $peer: $msg"
  }

  # Sends the given high-level message to the overseer.
  # If we are the overseer, receiveOverseer is called.
  method overseerMessage args {
    if {0 != [getOverseer]} {
      error "Networking not yet supported for overseerMessage"
    }

    receiveOverseer 0 $args
  }

  # Called when we are the overseer and receive an overseer-unicast
  # message from the given peer.
  # Default understands
  #   kill-notification remote-vpeer {killerpeer killervpeer} assist secondary0 secondary1
  #     ignored
  # Otherwise logs a warning that the message is not understood.
  # Overriders should always chain the method call on unknown messages.
  method receiveOverseer {peer msg} {
    lassign $msg type
    if {$type == "kill-notification"} return
    log "Warning: Unknown overseer-unicast received from $peer: $msg"
  }

  # END: OVERSEER AND MESSAGING INTERFACE

  # BEGIN: GUI EXTENSION

  # Populates the pause menu with options for the user.
  # The panel given is a VerticalContainer.
  # The pause menu gets normal focus; ie, mouse and keyboard
  # events go to the GUI instead of controlling the ship.
  # Overriders should chain this method.
  method populatePauseMenu panel {
    set unpause [new ::gui::Button [_ A game resume] "$this unpause"]
    $unpause setCancel
    $panel add $unpause
    $panel add [new ::gui::Button [_ A game exit] "$this finish"]
  }

  # Returns an AWidget containing game information.
  # This AWidget will be displayed over the game, but will NOT receive
  # focus; keyboard and mouse events will continue to control the ship.
  # Default returns an empty widget.
  # It is probably not meaningful to chain this call.
  method getStatsPanel {} {
    new ::gui::AWidget
  }

  # END: GUI EXTENSION

  # BEGIN: KEYBOARD CALLBACKS

  method ckc_exit {} {
    if {!$networkingEnabled} {
      set paused yes
    }
    # If unpauseMode is not empty, we have stats open.
    # Don't change unpauseMode, and delete the current mode.
    if {{} != $unpauseMode} {
      delete object $mode
    } else {
      set unpauseMode $mode
    }
    set mode [new BasicGamePauseMode $this]
  }

  # Toggles the given boolean variable if not networked
  private method toggleNotNetworked var {
    if {!$networkingEnabled} {
      set $var [expr "!$$var"]
    }
  }
  method ckc_frameXframe {} {
    toggleNotNetworked frameXframe
  }
  method ckc_fast {} {
    toggleNotNetworked fast
  }
  method ckc_slow {} {
    toggleNotNetworked slow
  }
  method ckc_halt {} {
    toggleNotNetworked paused
  }
  variable areStatsOn no
  method ckc_statsOn {} {
    if {$areStatsOn} return
    set areStatsOn yes
    # Use the "unpause" system for the same purpose
    set unpauseMode $mode
    set mode [new BasicGameMode $this [getStatsPanel]]
  }
  method ckc_statsOff {} {
    if {!$areStatsOn} return
    set areStatsOn no
    setMode $unpauseMode
    set unpauseMode {}
  }

  method unpause {} {
    set paused no
    setMode $unpauseMode
    set unpauseMode {}
  }

  # END: KEYBOARD CALLBACKS

  # BEGIN: DATA EXCHANGE METHODS

  # Perform the periodic querying of the overseer for data updates
  method queryOverseer {} {
    if {$networkingEnabled} {
      error "Networking not yet supported in queryOverseer"
    }
    # Currently does nothing
  }

  # Broadcast the datp deltata to all peers.
  method broadcastDatp {} {
    if {$networkingEnabled} {
      error "Networking not yet supported in broadcastDatp"
    }
    # Currently does nothing but clear the deltata
    set datpDeltata {}
  }

  # END: DATA EXCHANGE METHODS

  # BEGIN: RAW INPUT FORWARDING
  method keyboard evt {
    # Remember the old mode, so we can bail if it changes
    set oldmode $mode
    set fw [$mode enableRawInputForwarding]
    if {$fw && $human != "0"} {
      $human key $evt
    }
    if {$oldmode != $mode} return
    if {$fw != 1} {
      chain $evt
    }
  }

  method mouseButton evt {
    if {[$mode enableRawInputForwarding] && $human != "0"} {
      $human button $evt
    }
    if {[$mode enableRawInputForwarding] != 1} {
      chain $evt
    }
  }

  method motion evt {
    set ::cursorX [$evt cget -x]
    set ::cursorY [$evt cget -y]
    if {[$mode enableRawInputForwarding] && $human != "0"} {
      $human motion $evt
    }
    if {[$mode enableRawInputForwarding] != 1} {
      chain $evt
    }
  }
  # END: RAW INPUT FORWARDING
}

# Function object to use as shipExistenceFailure with BasicGame
class BasicGameShipDeathCallback {
  inherit fun<void:Ship*,bool>

  variable app

  constructor a {
    super fun<void:Ship*,bool> *default
  } {
    set app $a
  }

  method invoke {ship del} {
    $app shipDeath $ship $del
  }
}

# CommonKeyboardClient subclass to forward all methods to
# BasicGame::ckc_METHOD
class BasicGameCKC {
  inherit CommonKeyboardClient

  variable app

  constructor a {
    super CommonKeyboardClient *default
  } {
    set app $a
  }

  foreach method {exit frameXframe fast halt slow statsOn statsOff} {
    method $method args "\$app ckc_$method {*}\$args"
  }
}

# Primary mode for BasicGame which forwards raw events to the human controller.
# May display a centred component. This component MUST support compiled drawing:
# its draw method is never called.
class BasicGameMode {
  inherit ::gui::Mode

  variable app
  variable enableDrawing
  variable cursorEnabled

  # Constructs a new BasicGameMode.
  # bg: The BasicGame that owns it
  # centre: If non-empty, an AWidget to display in the centre of the screen
  constructor {bg centre} {
    ::gui::Mode::constructor
  } {
    set app $bg
    if {{} == $centre} {
      set centre [new ::gui::AWidget]
      set enableDrawing no
    } else {
      set enableDrawing yes
    }

    set root [new ::gui::ComfyContainer $centre]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
    $root pack

    acsgi_begin
    cglPushMatrix
    if {[$root minHeight] > $::vheight*0.9} {
      # Scale down
      set h [$root minHeight]
      set ratio [expr {$::vheight*0.9/$h}]
      # Move so still centred
      cglTranslate [expr {0.5-0.5*$ratio}] \
                   [expr {$::vheight/2-$::vheight/2*$ratio}]
      cglUScale $ratio
    }
    $root drawCompiled
    cglPopMatrix
    acsgi_end
    acsgi_textNormal no

    # Show cursor if either analogue setting is in joystick mode
    set scheme [$ str conf.control_scheme]
    if {![$ bool conf.$scheme.analogue.horiz.recentre]
    ||  ![$ bool conf.$scheme.analogue.vert.recentre]} {
      set cursorEnabled yes
    } else {
      set cursorEnabled no
    }
  }

  method draw {} {
    if {$enableDrawing} acsgi_draw
  }

  method enableRawInputForwarding {} {
    return 1
  }

  method enableCursor {} {
    return $cursorEnabled
  }
}

# Menu mode for BasicGame.
# Does not permit forwarding of raw events.
class BasicGamePauseMode {
  inherit ::gui::Mode

  variable app

  constructor a {
    ::gui::Mode::constructor
  } {
    set app $a

    set panel [new ::gui::VerticalContainer 0.01]
    $app populatePauseMenu $panel

    set root [new ::gui::ComfyContainer [new ::gui::Frame $panel]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
    $root pack
  }

  method enableRawInputForwarding {} {
    return 0
  }
}
