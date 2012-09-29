  class Application {
    inherit GameState
    protected variable subapp
    protected variable mode
    variable needWarpMouseX
    variable needWarpMouseY
    variable isWarpingMouse

    constructor {} { super GameState *default } {
      set subapp none
      set needWarpMouseX no
      set needWarpMouseY no
      set isWarpingMouse no
    }

    destructor {
      if {$subapp != "none"} {delete object $subapp}
      delete object $mode
    }

    method update et {
      global screenW screenH cursorX cursorY oldCursorX oldCursorY currentKBMods
      global lmeta rmeta lalt ralt lctrl rctrl lshift rshift
      global currentMode
      set currentMode $mode
      foreach obj $::gui::autodelete {
        delete object $obj
      }
      set ::gui::autodelete {}
      set rawmods [SDL_GetModState]
      set metamod  [expr {$rawmods & ($lmeta  | $rmeta )? "M" : "-"}]
      set altmod   [expr {$rawmods & ($lalt   | $ralt  )? "A" : "-"}]
      set ctrlmod  [expr {$rawmods & ($lctrl  | $rctrl )? "C" : "-"}]
      set shiftmod [expr {$rawmods & ($lshift | $rshift)? "S" : "-"}]
      set currentKBMods $metamod$altmod$ctrlmod$shiftmod
      if {$subapp != "none"} {
        set ret [$subapp update $et]
        if {$ret == $subapp} {
          delete object $subapp
          set subapp none
          configureGL
        } elseif {$ret != "0"} {
          delete object $subapp
          set subapp $ret
          $ret configureGL
        }
        return 0
      } else {
        ::update
        set ::gui::timeUntilNotificationClear \
          [expr {$::gui::timeUntilNotificationClear-min(256,$et)}]
        if {$::gui::timeUntilNotificationClear < 0} {
          if {[llength $::gui::notificationQueue]} {
            set ::gui::notificationQueue [lassign $::gui::notificationQueue ::gui::notification]
            set ::gui::timeUntilNotificationClear 4096
          } else {
            set ::gui::timeUntilNotificationClear 65536
            set ::gui::notification {}
          }
        }

        foreach colour {white standard warning danger special} {
          set ::gui::colourCache($colour) [::gui::getStdColourDirect $colour]
        }

        if {$::gui::needAcceleratorRefresh} {
          $mode refreshAccelerators
          set ::gui::needAcceleratorRefresh no
        }
        set cx [expr {$screenW/2}]
        set cy [expr {$screenH/2}]
        set isWarpingMouse yes
        switch $needWarpMouseX/$needWarpMouseY {
          no/no {}
          yes/no {
            SDL_WarpMouse $cx $cursorY
            set cursorX $cx
            set oldCursorX $cx
          }
          no/yes {
            SDL_WarpMouse $cursorX $cy
            set cursorY $cy
            set oldCursorY $cy
          }
          yes/yes {
            SDL_WarpMouse $cx $cy
            set cursorX $cx
            set oldCursorX $cx
            set cursorY $cy
            set oldCursorY $cy
          }
        }
        set isWarpingMouse no
        global applicationUpdateTime
        set applicationUpdateTime $et
        return [updateThis $et]
      }
    }

    # Update the application. This should be overridden instead
    # of the normal update method, since it forwards calls to any
    # subapp that may exist
    # Default does nothing.
    method updateThis et { return 0 }

    method draw {} {
      if {$subapp != "none"} {$subapp draw} \
      else {
        global currentMode
        set currentMode $mode
        set ::gui::cursorType [getCursor]
        drawThis ;# Background
        $mode draw
        if {[$mode enableNotifications] && 0 < [string length $::gui::notification]} {
          ::gui::colourStd
          $::gui::font preDraw
          $::gui::font drawStr $::gui::notification \
            [expr {1-[$::gui::font width $::gui::notification]}] 0
          $::gui::font postDraw
        }
        if {[$mode enableCursor]} {
          ::gui::drawCursor
        }
      }
    }

    # Draw the background (or only graphics) for this Application.
    # This should be overridden instead of the normal draw method,
    # since it forwards calls to any subapp that may exist.
    # Default does nothing.
    method drawThis {} {}

    # Return the current cursor to use.
    # Default returns crosshair.
    method getCursor {} { return crosshair }

    # Returns whether to disable keyboard input.
    # By default, returns true if the cursor is "busy".
    method disableKeyboardInput {} {
      expr {[getCursor] eq "busy"}
    }

    # Returns whether to disable mouse input.
    # By default, returns true if the cursor is "busy".
    method disableMouseInput {} {
      expr {[getCursor] eq "busy"}
    }

    method configureGL {} {
      if {$subapp != "none"} {$subapp configureGL} \
      else configureGLThis
    }

    # Configure Open GL. This method, instead of the normal configureGL,
    # should be used, as the normal is overridden to forward calls to
    # any subapp that may exist.
    # Default calls the default from GameState
    method configureGLThis {} {
      GameState::configureGL
    }

    # The GameState event handlers should not be used directly, except
    # when the actual event is needed to be passed back into C++
    method keyboard {evt} {
      if {$subapp != "none"} {
        $subapp keyboard $evt
        return
      }
      if {[disableKeyboardInput]} return
      global lshift rshift lctrl rctrl lalt ralt lmeta rmeta
      global currentKBMods
      global currentMode
      set currentMode $mode
      set keysym [$evt cget -keysym]
      set sym [$keysym cget -sym]
      set type [$evt cget -type]
      set rawmods [$keysym cget -mod]
      set metamod  [expr {$rawmods & ($lmeta  | $rmeta )? "M" : "-"}]
      set altmod   [expr {$rawmods & ($lalt   | $ralt  )? "A" : "-"}]
      set ctrlmod  [expr {$rawmods & ($lctrl  | $rctrl )? "C" : "-"}]
      set shiftmod [expr {$rawmods & ($lshift | $rshift)? "S" : "-"}]
      keyboardThis   $type:$metamod$altmod$ctrlmod$shiftmod:$sym
      $mode keyboard $type:$metamod$altmod$ctrlmod$shiftmod:$sym
      set currentKBMods $metamod$altmod$ctrlmod$shiftmod
      if {$type == "DOWN"} {
        characterThis   [toChar [$keysym cget -unicode]]
        $mode character [toChar [$keysym cget -unicode]]
      }
    }

    method keyboardThis spec {}
    method characterThis spec {}

    method motion {evt} {
      global currentMode
      set currentMode $mode
      if {$subapp != "none"} {
        $subapp motion $evt
        return
      }
      if {$isWarpingMouse} return
      global cursorX cursorY oldCursorX oldCursorY screenW screenH vheight
      # If the X or Y axes are locked, warp the mouse and ignore this
      # particular event
      if {$::gui::cursorLockX >= 0 && [$evt cget -x] != $::gui::cursorLockX} {
        SDL_WarpMouse $::gui::cursorLockX [$evt cget -y]
        return ;# Process event generated by warp instead
      }
      if {$::gui::cursorLockY >= 0 && [$evt cget -y] != $::gui::cursorLockY} {
        SDL_WarpMouse [$evt cget -x] $::gui::cursorLockY
        return ;# Process event generated by warp instead
      }
      set oldCursorX $cursorX
      set oldCursorY $cursorY
      set cursorX [$evt cget -x]
      set cursorY [$evt cget -y]
      if {[$mode xUsesRelativeMotion]} {
        set xarg [expr {$cursorX-$oldCursorX}]
      } else {
        set xarg $cursorX
      }
      if {[$mode yUsesRelativeMotion]} {
        set yarg [expr {$cursorY-$oldCursorY}]
      } else {
        set yarg $cursorY
      }
      if {[$mode xUsesScreenCoords]} {
        set xarg [expr {$xarg/double($screenW)}]
      }
      if {[$mode yUsesScreenCoords]} {
        set yarg [expr {1.0-$yarg/double($screenH)}]
        if {[$mode honourAspectRatio]} {
          set yarg [expr {$vheight*$yarg}]
        }
      }
      set states ""
      set buttonState [$evt cget -state]
      foreach {ix ch} {1 L 2 M 3 R 4 U 5 D} {
        append states [global SDL_BUTTON_$ix; expr {$buttonState & [set SDL_BUTTON_$ix]? $ch:"-"}]
      }
      if {![disableMouseInput]} {
        motionThis $xarg $yarg $states
        $mode motion $xarg $yarg $states
      }
      set needWarpMouseX [$mode xWarpMouse]
      set needWarpMouseY [$mode yWarpMouse]
    }

    method motionThis {x y states} {}

    method mouseButton {evt} {
      if {$subapp != "none"} {
        $subapp mouseButton $evt
        return
      }
      if {[disableMouseInput]} return
      global cursorX cursorY screenW screenH vheight
      global currentMode
      set currentMode $mode
      set xarg $cursorX
      set yarg $cursorY
      if {[$mode xUsesScreenCoords]} {
        set xarg [expr {$xarg/double($screenW)}]
      }
      if {[$mode yUsesScreenCoords]} {
        set yarg [expr {1.0-$yarg/double($screenH)}]
        if {[$mode honourAspectRatio]} {
          set yarg [expr {$vheight*$yarg}]
        }
      }

      set type [$evt cget -type]
      if {$type == "DOWN"} {
        if {"busy" == [getCursor]} return
      }
      set butt [$evt cget -button]
      # To be safe, disable axis locking whenever the left button is released
      if {$type == "UP" && $butt == "mb_left"} {
        set ::gui::cursorLockX -1
        set ::gui::cursorLockY -1
      }
      buttonThis   $type:$butt $xarg $yarg
      $mode button $type:$butt $xarg $yarg
    }

    method buttonThis {spec x y} {}
  }

