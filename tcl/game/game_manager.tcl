# The GameManager runs at a level above BasicGame, allowing it to handle
# concerns such as game mode switching.
#
# One of the primary concerns is the game mode strings. These have the
# following format:
#   MMMMCX options
# Where:
#   MMMM is the four-character English abbreviation, with number of teams
#   substituted. If the abbreviation is shorter than 4 characters, it is padded
#   with underscores to the right.
#   C is the single-character ship class restriction ('A', 'B', or 'C').
#   X is the class extension level. Currently it must be '0'.
# options is a Tcl dict describing any other parameters:
#   fieldw      The width of the field
#   fieldh      The height of the field
#
# If the underlying BasicGame (or other Application) exits voluntarily, the
# entire game ends.
#
# Currently understood modes (# indicates team count, 2--6):
#   DM__        Deathmatch
#   #TDM        Team deathmatch
#   LMS_        Last man standing
#   L#MS        Last team standing
#   HVC_        Humans vs Cyborgs
#   NULL        Intermission state
class GameManager {
  inherit ::gui::Application

  variable env
  variable network
  variable communicator
  variable standardHangar

  # Creates a new GameManager.
  #   networkGame       The NetworkGame instance to use, or 0 for a local game.
  #                     This object will be owned by the GameManager, and is
  #                     deleted when the GM is.
  #   initmeth          Method to call to start the game, with arguments (see
  #                     respective methods in this class).
  #   background        Code to evaluate to create the background object
  #   stdhangar         If true, make the appropriate AI best ships hangar
  #                     effective before creating each mode.
  constructor {networkGame initmeth background stdhangar} {
    set network $networkGame
    set standardHangar $stdhangar
    if {$network == 0} {
      set communicator [new LoopbackCommunicator 0]
    } else {
      set communicator [new NetworkCommunicator $network $this 0]
      $network setNetIface $communicator
    }

    set env [new GameEnv default 16 16]
    set field [$env getField]
    $env configure -stars [eval $background]
    if {$network != 0} {
      $network changeField $field
    }
    $this {*}$initmeth
    set mode [new ::gui::Mode]
  }

  destructor {
    if {$network != 0} {
      delete object $network
    }
    delete object $communicator
    delete object $env
  }

  method update {et} {
    if {$network != 0} {
      $network update [expr {int($et)}]
    }

    Application::update $et
    if {$subapp == "none"} {
      # Game mode exited, time to die
      return $this
    } else {
      return 0
    }
  }

  private method setsub {app} {
    if {$subapp != "none"} {
      delete object $subapp
    }
    set subapp $app
    $communicator set-game $subapp
  }

  private method modeError {modestr} {
    log "Unworkable mode string: $modestr"
    if {0 != $network} {
      delete object $network
      set network 0
    }
    setsub [new NetworkFailureApp [_ N protocol error]]
  }

  method networkError {what} {
    if {0 != $network} {
      delete object $network
      set network 0
    }
    setsub [new NetworkFailureApp $what]
  }

  private method createMode {modestr} {
    if {![string is list -strict $modestr]
    ||  2 != [llength $modestr]
    ||  6 != [string length [lindex $modestr 0]]
    ||  ![string is list [lindex $modestr 1]]
    ||  [llength [lindex $modestr 1]] % 2} {
      modeError $modestr
      return
    }

    set opt [lindex $modestr 1]
    if {[dict exists $opt desiredPlayers]
    &&  [string is integer -strict [dict get $opt desiredPlayers]]
    &&  [dict get $opt desiredPlayers] >= 0
    &&  [dict get $opt desiredPlayers] < 128} {
      set desiredPlayers [dict get $opt desiredPlayers]
    } else {
      set desiredPlayers 0
    }

    if {[dict exists $opt fieldw]} {
      set fw [dict get $opt fieldw]
      if {[string is integer -strict $fw] && $fw > 8 && $fw < 2048} {
        [$env getField] configure -width $fw
      }
    }
    if {[dict exists $opt fieldh]} {
      set fh [dict get $opt fieldh]
      if {[string is integer -strict $fh] && $fh > 8 && $fh < 2048} {
        [$env getField] configure -height $fh
      }
    }

    [$env getField] updateBoundaries

    set m [string range [lindex $modestr 0] 0 3]
    set cls [string index [lindex $modestr 0] 4]
    if {$cls ne "A" && $cls ne "B" && $cls ne "C"} {
      modeError $modestr
      return
    }

    if {$standardHangar} {
      makeHangarEffectiveByName hangar.user.bs$cls
    }

    switch -glob -- $m {
      NULL      {setsub [new NullNetworkState $network]}
      DM__      {setsub [new G_DM $desiredPlayers $env $communicator]}
      [2-6]TDM  {setsub [new G_XTDM $desiredPlayers [string index $m 0] \
                             $env $communicator]}
      LMS_      {setsub [new G_LMS $desiredPlayers $env $communicator]}
      L[2-6]TS  {setsub [new G_LXTS $desiredPlayers [string index $m 1] \
                             $env $communicator]}
      HVC_      {setsub [new G_HVC $desiredPlayers $env $communicator]}
      default   {modeError $modestr; return}
    }

    if {$network != 0} {
      $network updateFieldSize
      $communicator switchMode $modestr
    }
  }

  method init-lan {} {
    $network setLocalPeerNIDAuto
    $network setLocalPeerName $::abnet::username
  }

  # Initialising method which starts a new local game
  method init-local-game modestr {
    createMode $modestr
  }

  # Initialising method which stats a LAN game.
  method init-lan-game {ipv advertising modestr} {
    init-lan
    $network connectToNothing [expr {$ipv == 6}] true
    if {$advertising} {
      $network setAdvertising $modestr
    }

    createMode $modestr
  }

  # Initialising method which connects to the given discovered LAN game.
  method join-lan-game {advertising ix} {
    createMode "NULLC0 {}"

    init-lan
    $network connectToDiscovery $ix
    if {$advertising} {
      $network setAdvertising NULL
    }
  }

  # Initialising method which connects to the given IP address/port combo
  method join-private-game {advertising addr port} {
    createMode "NULLC0 {}"

    init-lan
    $network connectToLan $addr $port
    if {$advertising} {
      $network setAdvertising NULL
    }
  }

  method remote-swich-state {modestr} {
    log "remote-swich-state $modestr"
    createMode $modestr
  }
}
