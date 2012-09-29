  # A Mode defines the user-interface of the current Application.
  class Mode {
    # The AWidget that is the root for this mode
    protected variable root

    method fqn {} {return $this}

    constructor {} { set root {} }
    destructor {if {[string length $root]} {delete object $root}}

    method refreshAccelerators {} {
      $root setAccelerator [_ A gui accelerators]
    }

    # Return true if the mouse cursor should be shown,
    # false otherwise.
    # Default: yes
    method enableCursor {} {return yes}
    # Return true if notifications should be shown
    # Default: yes
    method enableNotifications {} {return yes}

    # Return true if the given axis uses relative mouse motion
    # events instead of absolute. This controls the arguments
    # passed to the motion method.
    # Default: no
    method xUsesRelativeMotion {} {return no}
    method yUsesRelativeMotion {} {return no}
    # Return true if coordinates should be in screens (ie, 0..1
    # and 0..vheight) instead of pixels (eg, 0..1023 and 0..767).
    # Default: yes
    method xUsesScreenCoords {} {return yes}
    method yUsesScreenCoords {} {return yes}
    # Returns true if the screen coords for Y honour aspect ratio
    # conversion. If true, the Y events range from 0 to vheight;
    # if false, 0 to 1.
    # Default: yes
    method honourAspectRatio {} {return yes}
    # Return true if the mouse is to be warped to the centre of
    # the given axis upon motion. Default returns the same
    # as UsesRelativeMotion.
    method xWarpMouse {} {xUsesRelativeMotion}
    method yWarpMouse {} {yUsesRelativeMotion}

    # Called on all SDL key events.
    # The argument is in the format
    #   {UP|DOWN}:{M|-}{A|-}{C|-}{S|-}:<sym>
    # where UP or DOWN indicates the key state, M indicates the
    # Meta modifier, A indicates the Alt modifier, C indicates
    # the Ctrl modifier, and S indicates the Shift modifier. Sym
    # is the keysym returned by the SDL_KeyEvent. For example,
    # Ctrl+Alt+Del would be represented as
    #   DOWN:-AC-:k_delete
    # followed eventually by
    #   UP:-AC-:k_delete
    # and Escape-Meta-Alt-Control-Shift is
    #   DOWN:MACS:k_escape
    # Default forwards everything to root.
    method keyboard keyspec {
      if {[string length $root]} {$root keyboard $keyspec}
    }

    # Called on SDL DOWN keyboard events,
    # with an integer representing the character produced.
    # Default forwards all to root
    method character ch {
      if {[string length $root]} {$root character $ch}
    }

    # Called on all SDL motion events that did
    # not originate from warping the mouse.
    # States represents the states of the five recognized
    # mouse buttons: LMRUD are present when the corresponding
    # button is pressed, or - otherwise (eg, L---- indicates
    # the left button alone is pressed).
    # Default forwards everything to root.
    method motion {x y states} {
      if {[string length $root]} {$root motion $x $y $states}
    }

    # Called on all SDL mouse button events.
    # Argument format is
    #   {UP|DOWN}:<button>
    # Where UP or DOWN indicates the state of the given
    # button. A left-click, for example, is
    #   DOWN:mb_left
    # followed by
    #   UP:mb_left
    # x and y are the same as for motion
    # Default forwards everything to root.
    method button {bspec x y} {
      if {[string length $root]} {$root button $bspec $x $y}
    }

    # Draw. Default just forwards to root.
    method draw {} {
      if {[string length $root]} {$root draw}
    }

    # Update. Not called by default Application.
    # Default does nothing.
    method update et {}
  }

