# Defines a Mode that can be used with BasicGame which allows the
# user to control a Spectator object and to see how long they must
# wait before respawning.

class SpectatorMode {
  inherit ::gui::Mode

  variable environment
  variable spectator
  variable messageCallback
  variable lblwho ;# The name of the ship we are spectating
  variable lblmessage ;# Message to show to the user

  # Constructs a new SpectatorMode with the given parms:
  # env         GameEnv to operate on
  # field       GameField the Spectator will live in
  # ship        Initial Ship, or 0
  # insta       Whether to instantly abandon the initial ship
  # callback    Evaluated each frame to get the message to show to the user
  constructor {env field ship insta callback} {
    ::gui::Mode::constructor
  } {
    set environment $env
    if {$ship != 0} {
      set spectator [new Spectator explicit $ship $insta]
    } else {
      set spectator [new Spectator empty $field]
    }
    $field add $spectator

    set messageCallback $callback

    set name {}
    if {$ship != 0} {
      set name [$ship cget -tag]
    }
    set lblwho [new ::gui::Label $name right]
    set lblmessage [new ::gui::Label {} right]

    set root [new ::gui::VerticalContainer 0.01 top]
    $root add $lblmessage
    $root add $lblwho
    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    $environment setReference $spectator no
  }

  destructor {
    # We need to ignore any error that occurs here, as we
    # could be called during destruction of the parent state,
    # in which case the specator is deleted befor us.
    # Allowing the error to propagate up will prevent {c++ delete}
    # from being called, and certain objects will never be removed
    # from the exports tables, eventually confusing the bridge and
    # causing the program to malfunction.
    catch {
      $spectator kill
    }
  }

  method draw {} {
    set ref [$spectator getReference]
    if {$ref != 0} {
      $lblwho setText [$ref cget -tag]
    }
    $lblmessage setText [namespace eval :: $messageCallback]
    chain
  }

  method backgroundUpdate et {
    # If the Spectator has a separate reference, follow that instead
    if {0 != [$spectator getReference]
    &&  [[$spectator getReference] hasPower]} {
      $environment setReference [$spectator getReference] no
    } else {
      $environment setReference $spectator no
    }
  }

  # When suspended, we must set the spectator as the reference, as we won't
  # be able to maintain the reference ourselves.
  # (This will have the effect of hiding the HUD when the stats panel or menu
  #  are brought up, but there isn't much we can do.)
  method suspend {} {
    $environment setReference $spectator no
  }

  method enableRawInputForwarding {} {
    # Both, unless writing to the composition buffer
    if {$::isCompositionBufferInUse} {
      return 1
    } else {
      return 2
    }
  }

  method enableCursor {} {
    return no
  }

  method keyboard evt {
    switch -glob -- $evt {
      DOWN:????:k_space         -
      DOWN:????:k_enter         {
        $spectator nextReference
      }
    }
  }

  method button {evt x y} {
    switch -glob -- $evt {
      DOWN:* {
        $spectator nextReference
      }
    }
  }

  method getSpectator {} {
    return $spectator
  }
}
