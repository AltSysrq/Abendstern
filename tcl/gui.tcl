package require Itcl
set applicationUpdateTime 0
# In case anyone needs the current modifiers, the default
# Application keyboard updates this
set currentKBMods ----

namespace eval gui {
  namespace import ::itcl::*
  set version 0.1

  # The standard font to use
  set font $::sysfont

  set needAcceleratorRefresh yes

  # The current cursor to use.
  # Currently supported are:
  #   crosshair         busy
  set cursorType crosshair

  # If non-negative, the cursor will be prevented from moving
  # away from this coordinate on the X axis.
  set cursorLockX -1
  # If non-negative, the cursor will be prevented from moving
  # away from this coordinate on the Y axis.
  set cursorLockY -1

  # All objects in this list are automatically deleted by the application
  # on the start of new frames
  set autodelete {}

  # Notification message to show at the bottom of the screen
  set notification {}
  set notificationQueue {}
  set timeUntilNotificationClear 0
  # Sets the current notification message
  proc setNotification str {
    if {"" == $::gui::notification} {
      set ::gui::notification $str
      set ::gui::timeUntilNotificationClear 4096
    } else {
      lappend ::gui::notificationQueue $str
    }
  }

  # This package defines the GUI system for Abendstern. The top level
  # is arranged into Applications, where an Application is anything like
  # the main menu, the game itself, the Ship editor, etc. An Application
  # is really a glorified GameState; it defines one or more Modes,
  # as well as a background draw operation. Each Mode defines the
  # following:
  # + More keybindings
  # + Whether the mouse cursor should be displayed
  # + How to handle mouse events
  # An Application has exactly one active Mode. It can also run a sub-
  # application, another Application that has full control until it
  # exits by returning itself from its update method.
  #
  # The rest of the GUI is formed via widgets (class AWidget), some of
  # which are containers.

  # Utility functions to set GL to standard colours
  proc getStdColourDirect {name {mult 1.0} {solid no}} {
    set base conf.hud.colours.$name
    list      [expr {[$ float $base.\[0\]]*$mult}] \
              [expr {[$ float $base.\[1\]]*$mult}] \
              [expr {[$ float $base.\[2\]]*$mult}] \
              [expr {$solid? 1.0 : [$ float $base.\[3\]]}]
  }
  proc getStdColour {name {mult 1.0} {solid no}} {
    if {![info exists ::gui::colourCache($name)]} {
      return [getStdColourDirect $name $mult $solid]
    }
    set cached $::gui::colourCache($name)
    list [expr {[lindex $cached 0]*$mult}] \
         [expr {[lindex $cached 1]*$mult}] \
         [expr {[lindex $cached 2]*$mult}] \
         [expr {$solid? 1.0 : [lindex $cached 3]}]
  }
  proc getColourStd  {{m 1.0} {s no}} { getStdColour standard $m $s }
  proc getColourWarn {{m 1.0} {s no}} { getStdColour warning  $m $s }
  proc getColourDang {{m 1.0} {s no}} { getStdColour danger   $m $s }
  proc getColourSpec {{m 1.0} {s no}} { getStdColour special  $m $s }
  proc getColourWhit {{m 1.0} {s no}} { getStdColour white    $m $s }
  proc colourStd   {{m 1.0} {s no}} { glColour {*}[getColourStd  $m $s]}
  proc colourWarn  {{m 1.0} {s no}} { glColour {*}[getColourWarn $m $s]}
  proc colourDang  {{m 1.0} {s no}} { glColour {*}[getColourDang $m $s]}
  proc colourSpec  {{m 1.0} {s no}} { glColour {*}[getColourSpec $m $s]}
  proc colourWhit  {{m 1.0} {s no}} { glColour {*}[getColourWhit $m $s]}
  proc ccolourStd  {{m 1.0} {s no}} {cglColour {*}[getColourStd  $m $s]}
  proc ccolourWarn {{m 1.0} {s no}} {cglColour {*}[getColourWarn $m $s]}
  proc ccolourDang {{m 1.0} {s no}} {cglColour {*}[getColourDang $m $s]}
  proc ccolourSpec {{m 1.0} {s no}} {cglColour {*}[getColourSpec $m $s]}
  proc ccolourWhit {{m 1.0} {s no}} {cglColour {*}[getColourWhit $m $s]}

  # Simple blending
  # Usage: getBlendedColours name mult name mult ...
  # ie, getBlendedColours standard 0.3 special 0.7
  proc getBlendedColours args {
    set blended {0.0 0.0 0.0 0.0}
    foreach {name mult} $args {
      set c [getStdColour $name $mult]
      for {set i 0} {$i < 4} {incr i} {
        set blended [lreplace $blended $i $i [expr {[lindex $blended $i]+[lindex $c $i]}]]
      }
    }
    return $blended
  }
  proc blendedColours args {
    eval "glColor4f [eval "getBlendedColours $args"]"
  }

  # Draw the cursor
  proc drawCursor {} {
    global cursorX cursorY screenW screenH vheight
    set drawX [expr {$cursorX/double($screenW)}]
    set drawY [expr {$vheight - $vheight*$cursorY/double($screenH)}]
    switch -exact -- $::gui::cursorType {
      crosshair {
        set dxl [expr {$drawX-0.01}]
        set dxr [expr {$drawX+0.01}]
        set dyb [expr {$drawY-0.01}]
        set dyt [expr {$drawY+0.01}]
        colourWhit
        glBegin GL_LINES
          glVertex2f $dxl $drawY
          glVertex2f $dxr $drawY
          glVertex2f $drawX $dyb
          glVertex2f $drawX $dyt
        glEnd
      }
      busy {
        # Hourglass (old-fassioned style)
        set dyt [expr {$drawY+0.01}]
        set dyb [expr {$drawY-0.01}]
        set dyht [expr {$drawY+0.005}]
        set dyhb [expr {$drawY-0.005}]
        set dxfl [expr {$drawX-0.0075}]
        set dxml [expr {$drawX-0.005}]
        set dxmr [expr {$drawX+0.005}]
        set dxfr [expr {$drawX+0.0075}]
        set dxhl [expr {$drawX-0.0025}]
        set dxhr [expr {$drawX+0.0025}]
        colourDang 1.0 yes ;# No transparency in cursor
        glBegin GL_TRIANGLES
          glVertex2f $drawX $drawY
          glVertex2f $dxhl $dyht
          glVertex2f $dxhr $dyht

          glVertex2f $dxml $dyb
          glVertex2f $dxmr $dyb
          glVertex2f $drawX $dyhb
        glEnd
        glBegin GL_LINES
          glVertex2f $drawX $drawY
          glVertex2f $drawX $dyhb
          colourWhit
          glVertex2f $dxfl $dyt
          glVertex2f $dxfr $dyt
          glVertex2f $dxfl $dyb
          glVertex2f $dxfr $dyb
          glVertex2f $dxml $dyt
          glVertex2f $dxmr $dyb
          glVertex2f $dxmr $dyt
          glVertex2f $dxml $dyb
          glVertex2f $dxml $dyt
          glVertex2f $dxml $dyb
          glVertex2f $dxmr $dyt
          glVertex2f $dxmr $dyb
        glEnd
      }
    }
  }

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
      if {"busy" == [getCursor]} return
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
      motionThis $xarg $yarg $states
      $mode motion $xarg $yarg $states
      set needWarpMouseX [$mode xWarpMouse]
      set needWarpMouseY [$mode yWarpMouse]
    }

    method motionThis {x y states} {}

    method mouseButton {evt} {
      if {$subapp != "none"} {
        $subapp mouseButton $evt
        return
      }
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

  # AWidget is the base class for all interface components in
  # the menu/HUD system.
  # The default AWidget is a "null widget": it performs no
  # actions, has an empty name, and requires no space.
  class AWidget {
    method fqn {} {return $this}

    # The current min/max borders of the widget
    protected variable left
    protected variable right
    protected variable top
    protected variable bottom

    # Used by the default implementation of button
    protected variable downWithin

    # The standard name of the widget. For example,
    # "Click here!"
    protected variable name
    # The display name of the widget. That is, the name
    # adapted to show the accelerator key. Examples:
    #   "\a[(white)C\a]lick here!"
    #   "C\a[(white)l\a]ick here!"
    #   "Click here! (\a[(white)X\a])"
    protected variable displayName
    # The accelerator, in key form (ie, "k_c" and not "C")
    protected variable accelerator

    constructor {} {
      set name ""
      set accelerator ""
      set downWithin no
    }

    # Request the widget's minimum width/height, respectively.
    # Default returns 0.
    method minWidth {} {return 0}
    method minHeight {} {return 0}

    # Returns the current dimensions
    method getWidth {} {
      expr {$right-$left}
    }
    method getHeight {} {
      expr {$top-$bottom}
    }

    # Return true if the widget is focussed; that is, accelerators
    # are ignored and all key events are passed to it.
    # Default returns no.
    method isFocused {} {return no}

    # Returns the current accelerator
    method getAccelerator {} {return $accelerator}
    # Sets the accelerator out of the given possibilities. The first
    # available character from the name of the widget; if none is
    # found, the first character from the possibilities is chosen
    # instead. Returns the possibilities after setting the accelerator.
    # Characters in the source name prefixed by \a& are copied to the
    # front of the name, and are given preference for highlighting.
    # This must also set the displayName appropriately.
    method setAccelerator possibilities {
      if {0==[string length $name]} {return $possibilities}
      # Strip out invalid characters and special sequences
      # Ex: "Click here!" --> "clickhere"
      set strippedName [regsub -all {\a(\[([^)]+|........)|\])} $name ""]
      while {-1 != [string first "\a&" $strippedName]} {
        set ix [string first "\a&" $strippedName]
        if {$ix+2 < [string length $strippedName]} {
          set strippedName \
            "[string index $strippedName [expr {$ix+2}]][string replace $strippedName $ix $ix+2]"
        }
      }
      set fromName [regsub -all "\[^[_ A gui accelerators]\]" [string tolower $strippedName] ""]
      for {set i 0} {$i < [string length $fromName]} {incr i} {
        set ch [string index $fromName $i]
        if {-1 != [string first $ch $possibilities]} {
          set chu [string toupper $ch]
          # OK, this is our accelerator
          # Highlight it in our display name
          # Prefer highlighting an explicit accelerator
          # Two cases (upper and lower)
          if {-1 != [set ix [string first "\a&$ch" $name]]} {
            set displayName [string replace $name $ix $ix+2 "\a\[(white)$ch\a\]"]
          } elseif {-1 != [set ix [string first "\a&$chu" $name]]} {
            set displayName [string replace $name $ix $ix+2 "\a\[(white)$chu\a\]"]
          # Prefer highlighting an upper case character (ie,
          # "Test <S>tate" is better than "Te<s>t State"
          } elseif {-1 != [set ix [string first $chu $name]]} {
            set displayName [string replace $name $ix $ix "\a\[(white)$chu\a\]"]
          } else {
            # No uppercase, so match lowercase instead
            set ix [string first $ch $name]
            set displayName [string replace $name $ix $ix "\a\[(white)$ch\a\]"]
          }

          # Remove from posibilities and return
          set accelerator $ch
          return [regsub $ch $possibilities ""]
        }
      }

      # No matches, take first possibility
      if {[string length $possibilities]} {
        set accelerator [string index $possibilities 0]
        set displayName "$name (\a\[(white)$accelerator\a\])"
        set ret [string range $possibilities 1 [string length $possibilities]]
        return $ret
      }

      # No more options (when would we EVER have more than
      # 36 widgets, though?)
      set accelerator ""
      set displayName $name
      return ""
    }

    # Returns true if the given coordinate is within the Widget
    method contains {x y} {
      expr {$x >= $left && $x < $right && $y >= $bottom && $y < $top}
    }

    # Called when the accelerator is tripped, or when the widget is
    # clicked.
    # Default does nothing.
    method action {} {}

    # Sets the size of the Widget
    method setSize {l r t b} {
      set left $l
      set right $r
      set bottom $b
      set top $t
    }

    # Draw the Widget
    method draw {} {}

    # Draw the Widget using ACSGI.
    # Only static widgets will actually support this.
    # Default does nothing.
    method drawCompiled {} {}

    # "Revert" the AWidget according to some
    # back storage. Default does nothing.
    method revert {} {}

    # "Save" the AWidget back into some
    # back storage. Default does nothing.
    method save {} {}

    # Called (with the same format as Mode::keyboard) on all keyboard
    # events, except when there is a focused widget. In that case,
    # only the focused widget receives the event.
    # Default does nothing
    method keyboard evt {}

    # Called for all character input events, except when there is a focused
    # widget. In that case, only the focused widget receives the event.
    # Default does calls action if the character matches the accelerator
    method character ch {
      if {[string tolower $ch] == $accelerator} action
    }

    # Called for all mouse button events, with the same format as
    # Mode::button (you probably want screen coords and preserved
    # srceen ratio for this to work correctly). The default
    # calls action upon UP:mb_left if both that event and the
    # previous DOWN:mb_left occurred within the component.
    method button {bspec x y} {
      switch $bspec {
        DOWN:mb_left {set downWithin [contains $x $y]}
        UP:mb_left {
          if {$downWithin && [contains $x $y]} {
            action
          }
          set downWithin no
        }
      }
    }

    # Called for all mouse motion events.
    # Default does nothing.
    method motion {x y states} {}
  }

  # A Container is an AWidget that holds other AWidgets.
  # This class implements all functionality necessary for
  # this, EXCEPT for the setSize method (during which layout
  # must be done) the abstract getMembers method, which
  # subclasses must override to return the list of members
  # (most subclasses will also need an add method).
  # The minWidth and minHeight must also be defined by
  # subclasses.
  # The Container's destructor automatically deletes all
  # its members.
  class Container {
    inherit AWidget

    constructor {} {AWidget::constructor} {}
    destructor {
      foreach mem [getMembers] {
        delete object $mem
      }
    }

    # Returns a list of AWidgets that are contained
    # by this Container
    method getMembers {}

    # isFocused returns true if ANY of its members
    # are focused
    method isFocused {} {
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          return yes
        }
      }
      return no
    }

    # setAccelerator calls the method for all subclasses, while
    # leaving its own untouched (it has no accelerator).
    method setAccelerator p {
      foreach mem [getMembers] {
        set p [$mem setAccelerator $p]
      }
      return $p
    }

    # The default draw just calls same for all members
    method draw {} {
      foreach mem [getMembers] {
        $mem draw
      }
    }

    method drawCompiled {} {
      foreach mem [getMembers] {
        $mem drawCompiled
      }
    }

    method revert {} {
      foreach mem [getMembers] {
        $mem revert
      }
    }

    method save {} {
      foreach mem [getMembers] {
        $mem save
      }
    }

    # The event handlers just forward things to
    # members, honouring the focus requirements
    method keyboard evt {
      # First, see if anyone's focused; if so,
      # pass the event and return
      # Otherwise, tell everybody about it
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          $mem keyboard $evt
          return
        }
      }
      foreach mem [getMembers] {
        $mem keyboard $evt
      }
    }

    method character ch {
      # Only pass to focused member if there are any
      foreach mem [getMembers] {
        if {[$mem isFocused]} {
          $mem character $ch
          return
        }
      }
      foreach mem [getMembers] {
        $mem character $ch
      }
    }

    method motion {x y states} {
      foreach mem [getMembers] {
        $mem motion $x $y $states
      }
    }

    method button {evt x y} {
      foreach mem [getMembers] {
        $mem button $evt $x $y
      }
    }
  }

  # A Frame is a Container that holds a single AWidget, but
  # draws a border around it and a background behind it
  class Frame {
    inherit Container

    variable w
    constructor onlyChild {Container::constructor} { set w $onlyChild }
    method getMembers {} { return $w }
    method setSize {a b c d} {
      AWidget::setSize $a $b $c $d
      $w setSize $a $b $c $d
    }
    method minWidth {} {$w minWidth}
    method minHeight {} {$w minHeight}
    method draw {} {
      # Gradient from dim std to transparent black
      glBegin GL_QUADS
        ::gui::colourStd 0.5
        glVertex2f $left $top
        glVertex2f $right $top
        glColor4f 0 0 0 0
        glVertex2f $right $bottom
        glVertex2f $left $bottom
      glEnd
      # Frame
      ::gui::colourStd
      glBegin GL_LINE_STRIP
        glVertex2f $left $top
        glVertex2f $right $top
        glVertex2f $right $bottom
        glVertex2f $left $bottom
        glVertex2f $left $top
      glEnd

      Container::draw
    }

    method drawCompiled {} {
      cglBegin GL_QUADS
        ::gui::ccolourStd 0.5
        cglVertex $left $top
        cglVertex $right $top
        cglColour 0 0 0 0
        cglVertex $right $bottom
        cglVertex $left $bottom
      cglEnd
      ::gui::ccolourStd
      cglBegin GL_LINE_STRIP
        cglVertex $left $top
        cglVertex $right $top
        cglVertex $right $bottom
        cglVertex $left $bottom
        cglVertex $left $top
      cglEnd

      Container::drawCompiled
    }
  }

  # A ComfyContainer houses a single AWidget, whose size
  # is shrunken by a given factor. Both factors may be
  # controlled, but the Y factor defaults to X's
  class ComfyContainer {
    inherit Container
    variable w
    variable xfactor
    variable yfactor
    constructor {child {xfac 0.8} {yfac -1}} {
      Container::constructor
    } {
      set w $child
      set xfactor $xfac
      if {$yfac < 0} {
        set yfactor $xfac
      } else {
        set yfactor $yfac
      }
    }

    method getMembers {} {return $w}
    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set cx [expr {($l+$r)/2.0}]
      set cy [expr {($t+$b)/2.0}]
      set rx [expr {($r-$l)/2.0}]
      set ry [expr {($t-$b)/2.0}]
      $w setSize [expr {$cx-$rx*$xfactor}] [expr {$cx+$rx*$xfactor}] \
                 [expr {$cy+$ry*$yfactor}] [expr {$cy-$ry*$yfactor}]
    }
    method minWidth {} {$w minWidth}
    method minHeight {} {$w minHeight}

    # Resets the factors to match the minimum size of the inner
    # component.
    method pack {} {
      packx
      packy
    }

    # Packs only the X dimension
    method packx {} {
      set xfactor [expr {[minWidth]/($right-$left)}]
      setSize $left $right $top $bottom
    }

    # Packs only the Y dimension
    method packy {} {
      set yfactor [expr {[minHeight]/($top-$bottom)}]
      setSize $left $right $top $bottom
    }
  }

  # A BorderContainer holds up to 5 widgets, in the following
  # layout:
  #   [___Top___]
  #   |L|     |R|
  #   |e| Ce  |i|
  #   |f| nt  |g|
  #   |t| re  |h|
  #   | |_____|t|
  #   [_Bottom__]
  # In words:
  #   The top and bottom's heights are exactly their minimum heights.
  #   The left and right's widths are exactly their minimum widths.
  #   The minimum height is h(top)+h(bottom)+max(h(left),h(right),h(centre))
  #   The minimum width is w(left)+w(right)+max(w(top),w(bottom),w(centre))
  #   The width of the top and bottom is the width of the entire widget.
  #   The width of the centre is width-w(left)-w(right)
  #   The height of the left, right, and centre is height-h(top)-h(bottom).
  # The order of the widgets is top, left, centre, right, bottom, unless
  # the makeCentreLast method is called, in which case centre is moved to
  # the end.
  class BorderContainer {
    inherit Container

    variable top
    variable left
    variable centre
    variable right
    variable bottom
    variable xshim
    variable yshim
    variable centreLast

    constructor {{xs 0} {ys 0}} {Container::constructor} {
      foreach v {top left centre right bottom} {
        set $v [new ::gui::AWidget]
      }
      set xshim $xs
      set yshim $ys
      set centreLast no
    }

    method getMembers {} {
      if {$centreLast} {
        list $top $left $right $bottom $centre
      } else {
        list $top $left $centre $right $bottom
      }
    }

    method makeCentreLast {} { set centreLast yes }

    method minWidth {} {
      expr {[$left minWidth]+[$right minWidth]+
            max([$top minWidth],[$centre minWidth],[$bottom minWidth])+2*$xshim}
    }
    method minHeight {} {
      expr {[$top minHeight]+[$bottom minHeight]+
            max([$left minHeight],[$centre minHeight],[$bottom minHeight])+2*$yshim}
    }
    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set w [expr {$r-$l}]
      set h [expr {$t-$b}]
      set tl [expr {$t-[$top minHeight]}]
      set bh [expr {$b+[$bottom minHeight]}]
      set lr [expr {$l+[$left minWidth]}]
      set rl [expr {$r-[$right minWidth]}]
      $top setSize $l $r $t $tl
      $left setSize $l $lr [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $centre setSize [expr {$lr+$xshim}] [expr {$rl-$xshim}] [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $right setSize $rl $r [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $bottom setSize $l $r $bh $b
    }

    # Set the given element. The old value will be deleted first.
    # We also honour Java-style compass elements.
    method setElt {elt val} {
      switch -- $elt {
        top - north {set e top}
        left - west {set e left}
        centre - center {set e centre}
        right - east {set e right}
        bottom - south {set e bottom}
        default { error "Unrecognized element: $elt" }
      }
      delete object [set $e]
      set $e $val
    }
  }

  # The DividedContainer contains exactly three widgets: left, centre,
  # and right. The centre widget is exactly its minimum width and is
  # perfectly centred within the container, and the left and right
  # widgets fill the rest of the space on the sides. All widgets have
  # the same height as the container. The minimum width is the minimum
  # width of the centre plus the maximum of the minima of the sides.
  # The minimum height is the greatest minimum height of any widget.
  class DividedContainer {
    inherit Container
    variable left
    variable centre
    variable right
    variable shim

    constructor {l c r {xs 0}} {
      Container::constructor
    } {
      set left $l
      set centre $c
      set right $r
      set shim $xs
    }

    method getMembers {} {
      list $left $centre $right
    }

    method minWidth {} {
      expr {2*$shim + [$centre minWidth] + 2*max([$left minWidth], [$right minWidth])}
    }

    method minHeight {} {
      expr {max(max([$left minHeight], [$centre minHeight]), [$right minHeight])}
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set centreW [$centre minWidth]
      $centre setSize [expr {($l+$r)/2.0-$centreW/2.0}] \
                      [expr {($l+$r)/2.0+$centreW/2.0}] \
                      $t $b
      $left  setSize $l [expr {($l+$r)/2.0-$centreW/2.0-$shim}] $t $b
      $right setSize [expr {($l+$r)/2.0+$centreW/2.0+$shim}] $r $t $b
    }
  }

  # The VerticalContainer stacks its elements vertically.
  # The minimum width is the maximum minimum width of any element.
  # The minimum height is the sum of the minimum heights of the elements,
  # plus (n-1)*shim.
  # The width of each element is the width of the container.
  # The height of each element is the minimum height of the widget; this
  # means there may be left-over space at the bottom. The exception is when
  # the container is in grid mode, in which case all have the height
  # h/count.
  class VerticalContainer {
    inherit Container

    variable members
    variable shim
    variable mode

    constructor {{s 0} {mod top}} {Container::constructor} {
      set members {}
      set shim $s
      set mode $mod
    }

    method getMembers {} {return $members}
    method minWidth {} {
      set max 0
      foreach mem $members {
        set w [$mem minWidth]
        if {$w>$max} {set max $w}
      }
      return $max
    }

    method minHeight {} {
      if {$mode != "grid"} {
        set sum 0
        foreach mem $members {
          set sum [expr {$sum+[$mem minHeight]}]
        }
        return [expr {$sum+$shim*([llength $members]-1)}]
      } else {
        set max 0
        foreach mem $members {
          set h [$mem minHeight]
          if {$h > $max} { set max $h }
        }
        return [expr {($shim+$max)*[llength $members] - $shim}]
      }
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set getMinHeight {$mem minHeight}
      switch $mode {
        top {set y $t}
        bottom {set y [expr {$b+[minHeight]}]}
        centre -
        center {set y [expr {($t+$b)/2.0+[minHeight]/2.0}]}
        grid {set y $t; set getMinHeight "expr {($t-$b)/double([llength $members])}"}
      }
      foreach mem $members {
        set mh [eval $getMinHeight]
        set ny [expr {$y-$mh}]
        $mem setSize $l $r $y $ny
        set y [expr {$ny-$shim}]
      }
    }

    method add w {
      lappend members $w
    }
  }

  # A HorizontalContainer is in every way identical to
  # a VerticalContainer, except operating on the X axis
  class HorizontalContainer {
    inherit Container
    variable members
    variable shim
    variable mode
    constructor {{s 0} {mod left}} {Container::constructor} {
      set memberl {}
      set shim $s
      set mode $mod
    }

    method getMembers {} {return $members}
    method minWidth {} {
      if {$mode != "grid"} {
        set sum 0
        foreach mem $members {
          set sum [expr {$sum+[$mem minWidth]}]
        }
        return [expr {$sum+$shim*([llength $members]-1)}]
      } else {
        set max 0
        foreach mem $members {
          set w [$mem minWidth]
          if {$w > $max} { set max $w }
        }
        return [expr {($shim+$max)*[llength $members] - $shim}]
      }
    }

    method minHeight {} {
      set max 0
      foreach mem $members {
        set h [$mem minHeight]
        if {$h>$max} {set max $h}
      }
      return $max
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set getMinWidth {$mem minWidth}
      switch $mode {
        left {set x $l}
        right {set x [expr {$r-[minWidth]}]}
        centre -
        center {set x [expr {($l+$r)/2.0-[minWidth]/2.0}]}
        grid {set x $l; set getMinWidth "expr {($r-$l)/double([llength $members])-$shim}"}
      }
      foreach mem $members {
        set nx [expr {$x+[eval $getMinWidth]}]
        $mem setSize $x $nx $t $b
        set x [expr {$nx+$shim}]
      }
    }

    method add w {
      lappend members $w
    }
  }

  # A StackContainer simply stacks all its components.
  # It generally should only be used to stack non-interactive
  # widgets, except for the top layer.
  class StackContainer {
    inherit Container

    variable members

    constructor {} {Container::constructor} {
      set members {}
    }

    method getMembers {} {return $members}
    method minWidth {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minWidth])}]
      }
      return $max
    }
    method minHeight {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minHeight])}]
      }
      return $max
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      foreach mem $members {
        $mem setSize $l $r $t $b
      }
    }

    method add w {
      lappend members $w
    }
  }

  # A Label simply displays fixed text. It does not take an accelerator.
  # It can be left-, centre-, or right-oriented.
  # It is always centred vertically.
  class Label {
    inherit AWidget
    variable orient

    constructor {text {orientation centre}} {AWidget::constructor} {
      setText $text
      setOrientation $orientation
    }

    method getText {} {return $name}
    method setText t {
      set name $t
      set displayName $t
    }
    method getOrientation {} {
      return $orient
    }
    method setOrientation or {
      switch -- $or {
        left - right - centre {set orient $or}
        center {set orient centre}
        default {error "Unknown orientation: $or"}
      }
    }

    method minHeight {} {
      $::gui::font getHeight
    }
    method minWidth {} {
      $::gui::font width $displayName
    }

    method setAccelerator p { return $p }

    method draw {} {
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set y [expr {$bottom+$h/2.0-[minHeight]/2.0}]
      switch $orient {
        left {set x $left}
        right {set x [expr {$right-[minWidth]}]}
        centre {set x [expr {$left+$w/2.0-[minWidth]/2.0}]}
      }
      set f $::gui::font
      ::gui::colourStd
      $f preDraw
      $f drawStr $displayName $x $y
      $f postDraw
    }

    method drawCompiled {} {
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set y [expr {$bottom+$h/2.0-[minHeight]/2.0}]
      switch $orient {
        left {set x $left}
        right {set x [expr {$right-[minWidth]}]}
        centre {set x [expr {$left+$w/2.0-[minWidth]/2.0}]}
      }
      ::gui::ccolourStd
      cglText $displayName $x $y
    }
  }

  # A MultiLineLabel acts much like a label, but wraps its text.
  # It is always left- and top-alligned.
  class MultiLineLabel {
    inherit AWidget

    variable text
    variable vminHeight

    constructor {{inittext {}} {lines 1}} {
      AWidget::constructor
    } {
      set text $inittext
      set vminHeight [expr {[$::gui::font getHeight]*$lines}]
    }

    method getText {} { return $text }
    method setText txt {
      set text $txt
    }

    method minHeight {} {
      return $vminHeight
    }

    method setMinHeight h {
      set vminHeight $h
    }

    method draw {} {
      set h [$::gui::font getHeight]
      set y [expr {$top - $h}]
      set width [expr {$right-$left}]
      set lines [split $text "\n"]
      ::gui::colourStd
      $::gui::font preDraw
      set currLine {}

      foreach line $lines {
        if {![string length [string trim $line]]} continue
        set words [split $line]
        set currLine {}

        foreach word $words {
          set word [string trim $word]
          if {![string length $word]} continue
          if {[$::gui::font width "$currLine $word"] > $width} {
            $::gui::font drawStr $currLine $left $y
            set y [expr {$y-$h}]
            set currLine $word
          } elseif {[string length $currLine]} {
            set currLine "$currLine $word"
          } else {
            set currLine $word
          }
        }

        if {[string length $currLine]} {
          $::gui::font drawStr $currLine $left $y
          set y [expr {$y-$h}]
          set currLine {}
        }
      }
      if {[string length $currLine]} {
        $::gui::font drawStr $currLine $left $y
        set y [expr {$y-$h}]
      }
      $::gui::font postDraw
    }
  }

  # A Button is a Widget that runs a given Tcl script in the root namespace
  # when its action is triggered
  class Button {
    inherit AWidget

    variable shim
    variable script
    # When action is called, this is set to 1
    # and reduced back to 0 over half a second
    protected variable highlighting

    variable isDefault
    variable isCancel
    variable isLeft
    variable isRight

    constructor {nam scr} {
      AWidget::constructor
    } {
      set name $nam
      set script $scr
      set highlighting 0
      set isDefault no
      set isCancel no
      set isLeft no
      set isRight no
      set shim [expr {[$::gui::font getHeight]*0.3}]
    }

    method getText {} {return $name}
    method setText nam {set name $nam; setAccelerator $accelerator}
    method getScript {} {return $script}
    method setScript s {set script $s}

    # After calling this, the button will respond
    # to the enter key
    method setDefault {} {set isDefault yes}
    # After calling this, the button will respond
    # to the escape key
    method setCancel {} {set isCancel yes}
    # Cause the button to respond to the left/right keys (depending
    # on which method is called). This disables the normal accelerator,
    # so the button should have an intuitive "name" (like "<<" or ">>").
    method setLeft {} {set isLeft yes}
    method setRight {} {set isRight yes}

    method setAccelerator p {
      if {!$isLeft && !$isRight} {AWidget::setAccelerator $p} \
      else {
        set displayName $name
        return $p
      }
    }

    method minHeight {} {
      expr {[$::gui::font getHeight]+$shim*2}
    }

    method minWidth {} {
      expr {[$::gui::font width $displayName]+$shim*2}
    }

    method action {} {
      set highlighting 1.0
      namespace eval :: $script
    }

    # Override key handling to check for default/cancel
    method keyboard kspec {
      switch -glob $kspec {
        DOWN:????:k_enter  { if {$isDefault} action }
        DOWN:????:k_escape { if {$isCancel } action }
        DOWN:????:k_left   { if {$isLeft   } action }
        DOWN:????:k_right  { if {$isRight  } action }
        default { AWidget::keyboard $kspec }
      }
    }

    method draw {} {
      global applicationUpdateTime
      # Update highlighting
      if {$highlighting > 0} {
        set highlighting [expr {max(0.0,$highlighting-0.002*$applicationUpdateTime)}]
      }

      # Draw background gradient first
      ::gui::blendedColours special $highlighting standard [expr {(1-$highlighting)/2.0}]
      glBegin GL_QUADS
        glVertex2f $left $top
        glVertex2f $right $top
        glColor4f 0 0 0 0
        glVertex2f $right $bottom
        glVertex2f $left $bottom
      glEnd

      # Draw border. Special if default, warning if cancel, standard otherwise
      if {$isDefault} ::gui::colourSpec elseif {$isCancel} ::gui::colourWarn else ::gui::colourStd
      glBegin GL_LINE_STRIP
        glVertex2f $left $top
        glVertex2f $right $top
        glVertex2f $right $bottom
        glVertex2f $left $bottom
        glVertex2f $left $top
      glEnd

      # Draw name, centred
      set f $::gui::font
      set x [expr {($right+$left)/2.0-[minWidth ]/2.0+$shim}]
      set y [expr {($top+$bottom)/2.0-[minHeight]/2.0+$shim}]
      ::gui::colourStd
      $f preDraw
      $f drawStr $displayName $x $y
      $f postDraw
    }
  }

  # The TabPanel is an AWidget that holds one or more AWidgets, each
  # assigned names. Only one of these AWidgets is shown at a time;
  # the user can select them via the buttons to the left.
  # A TabPanel may also have its tabs along the top, by passing the tabsOnTop
  # parameter to the constructor as true.
  class TabPanel {
    inherit BorderContainer

    variable buttonBar
    variable viewPane

    constructor {{tabsOnTop no}} {
      BorderContainer::constructor 0.01
    } {
      if {$tabsOnTop} {
        set buttonBar [new ::gui::HorizontalContainer 0 left]
        setElt top $buttonBar
      } else {
        set buttonBar [new ::gui::VerticalContainer 0 grid]
        setElt left $buttonBar
      }
      set viewPane [new ::gui::TabPanelViewer]
      setElt centre $viewPane
    }

    # Adds a new tab
    method add {name content} {
      $buttonBar add [new ::gui::TabPanelButton $name $viewPane $content]
      $viewPane add $content
    }

    # Returns the currently active component
    method getActive {} {
      $viewPane getActive
    }

    method setActive which {
      [lindex [$buttonBar getMembers] $which] action
    }
  }

  # For internal use by TabPanel.
  # A simple AWidget that forwards everything to its active
  # item. The minWidth and minHeight are the maxima of its
  # contents.
  class TabPanelViewer {
    inherit AWidget

    variable members
    variable active
    constructor {} {
      AWidget::constructor
    } {
      set members {}
    }

    method minWidth {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minWidth])}]
      }
      return $max
    }
    method minHeight {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minHeight])}]
      }
      return $max
    }

    method isFocused {} {
      $active isFocused
    }

    method getAccelerator {} {
      $active getAccelerator
    }

    method setAccelerator p {
      $active setAccelerator $p
    }

    method setSize {a b c d} {
      AWidget::setSize $a $b $c $d
      $active setSize $a $b $c $d
    }

    method revert {} {
      foreach mem $members {
        $mem revert
      }
    }

    method save {} {
      foreach mem $members {
        $mem save
      }
    }

    method draw {} {
      $active draw
    }

    method keyboard evt {
      $active keyboard $evt
    }

    method motion {x y states} {
      $active motion $x $y $states
    }

    method button {evt x y} {
      $active button $evt $x $y
    }

    method character evt {
      $active character $evt
    }

    method getActive {} { return $active }
    method setActive a {
      set active $a
      global currentMode
      $currentMode refreshAccelerators
      $active setSize $left $right $top $bottom
    }

    method setActiveAt a {
      setActive [lindex $members $a]
    }

    method add a {
      lappend members $a
      if {1 == [llength $members]} {
        set active $a
      }
    }

    destructor {
      foreach mem $members {
        delete object $mem
      }
    }
  }

  # For private use by TabPanel.
  # Extends button to use highlighting to indicate
  # current tab.
  class TabPanelButton {
    inherit Button
    variable activatee
    variable pane

    constructor {name pan content} {
      Button::constructor $name "$pan setActive $content"
    } {
      set activatee $content
      set pane $pan
    }

    method draw {} {
      if {$activatee == [$pane getActive]} {set highlighting 1}
      Button::draw
    }
  }

  # The base class for Checkbox and RadioButton.
  class AbstractCheckbox {
    inherit AWidget

    protected variable checked
    protected variable checkAction
    protected variable uncheckAction

    constructor {nam {ca ""} {uca ""}} {
      AWidget::constructor
    } {
      set name $nam
      set checkAction $ca
      set uncheckAction $uca
    }

    method minHeight {} {
      $::gui::font getHeight
    }

    method minWidth {} {
      expr {[$::gui::font getHeight]*1.5+[$::gui::font width $displayName]}
    }

    method isChecked {} {
      return $checked
    }

    # Subclasses must override this to draw the box and
    # then call this method. The current GL colour is
    # used for drawing the text, so the subclass must
    # set this appropriately.
    method draw {} {
      set f $::gui::font
      set x [expr {$left+[$f getHeight]*1.5}]
      set y [expr {($top+$bottom-[$f getHeight])/2}]
      $f preDraw
      $f drawStr $displayName $x $y
      $f postDraw
    }
  }

  # A Checkbox represents an individual boolean setting.
  # This setting is accessed and saved via code provided
  # at the constructor. The loader takes no arguments and
  # returns a boolean. The saver has the checked state
  # appended to it.
  # It is also possible to detect immediate checks/unchecks
  # by passing check/uncheck actions
  class Checkbox {
    inherit AbstractCheckbox

    variable howToLoad
    variable howToSave

    constructor {name {loader {}} {saver {}} {checkAction ""} {uncheckAction ""}} {
      AbstractCheckbox::constructor $name $checkAction $uncheckAction
    } {
      set howToLoad $loader
      set howToSave $saver
      set checked no
      revert ;#Load the current setting
    }

    method revert {} {
      if {"" != $howToLoad} {
        set checked [uplevel #0 $howToLoad]
      }
    }

    method save {} {
      if {"" != $howToSave} {
        uplevel #0 "$howToSave $checked"
      }
    }

    method action {} {
      if {$checked} {
        set checked no
        uplevel #0 $uncheckAction
      } else {
        set checked yes
        uplevel #0 $checkAction
      }
    }

    method draw {} {
      # Outline of box in std; if checked,
      # solid special square within
      ::gui::colourStd
      set sz [expr {[$::gui::font getHeight]/2}]
      set cx [expr {$left+$sz}]
      set cy [expr {($top+$bottom)/2}]
      glBegin GL_LINE_STRIP
        glVertex2f [expr {$cx-$sz}] [expr {$cy-$sz}]
        glVertex2f [expr {$cx-$sz}] [expr {$cy+$sz}]
        glVertex2f [expr {$cx+$sz}] [expr {$cy+$sz}]
        glVertex2f [expr {$cx+$sz}] [expr {$cy-$sz}]
        glVertex2f [expr {$cx-$sz}] [expr {$cy-$sz}]
      glEnd

      if {$checked} {
        ::gui::colourSpec
        set sz [expr {$sz*0.8}]
        glBegin GL_QUADS
          glVertex2f [expr {$cx-$sz}] [expr {$cy-$sz}]
          glVertex2f [expr {$cx-$sz}] [expr {$cy+$sz}]
          glVertex2f [expr {$cx+$sz}] [expr {$cy+$sz}]
          glVertex2f [expr {$cx+$sz}] [expr {$cy-$sz}]
        glEnd
        ::gui::colourStd ;#Draw the text normally
      }

      AbstractCheckbox::draw
    }

    # Explicitly sets the checked state of the checkbox
    # Calls action only if the state changes
    method setChecked ch {
      if {($checked && !$ch) || (!$checked && $ch)} action
    }

    # Explicitly sets the checked state without triggering
    # any actions
    method setCheckedNoAction ch {
      set checked $ch
    }
  }

  # RadioButtons are like mutually-exclusive Checkboxen.
  # They are linked together in a group, and activating
  # any one of them deactivates all the others.
  # Code-wise, behaviour is much the same as Checkbox,
  # except that the save code is called ONLY for the
  # checked RadioButton, and has nothing appended.
  class RadioButton {
    inherit AbstractCheckbox

    variable howToLoad
    variable howToSave
    public variable next
    public variable previous

    constructor {name {loader {}} {saver {}} {prev none} {checkAction ""} {uncheckAction ""}} {
      AbstractCheckbox::constructor $name $checkAction $uncheckAction
    } {
      set howToLoad $loader
      set howToSave $saver
      set previous $prev
      set next none
      set checked no
      if {$prev != "none"} {
        $prev configure -next $this
      }
      revert
    }

    method revert {} {
      if {"" != $howToLoad} {
        set checked [uplevel #0 $howToLoad]
      }
    }

    method save {} {
      if {"" != $howToSave} {
        if {$checked} {uplevel #0 $howToSave}
      }
    }

    method action {} {
      if {!$checked} {
        set checked yes
        uplevel #0 $checkAction
        if {$previous != "none"} {
          $previous uncheckPrevious
        }
        if {$next != "none"} {
          $next uncheckNext
        }
      }
    }

    method uncheckPrevious {} {
      if {$checked} {
        uplevel #0 $uncheckAction
        set checked no
      } elseif {$previous != "none"} {
        $previous uncheckPrevious
      }
    }

    method uncheckNext {} {
      if {$checked} {
        uplevel #0 $uncheckAction
        set checked no
      } elseif {$next != "none"} {
        $next uncheckNext
      }
    }

    method draw {} {
      # Outline of diamond in std; if checked,
      # solid special diamond within
      ::gui::colourStd
      set sz [expr {[$::gui::font getHeight]/2}]
      set cx [expr {$left+$sz}]
      set cy [expr {($top+$bottom)/2}]
      glBegin GL_LINE_STRIP
        glVertex2f [expr {$cx-$sz}] [expr {$cy}]
        glVertex2f [expr {$cx}] [expr {$cy+$sz}]
        glVertex2f [expr {$cx+$sz}] [expr {$cy}]
        glVertex2f [expr {$cx}] [expr {$cy-$sz}]
        glVertex2f [expr {$cx-$sz}] [expr {$cy}]
      glEnd

      if {$checked} {
        ::gui::colourSpec
        set sz [expr {$sz*0.8}]
        glBegin GL_QUADS
          glVertex2f [expr {$cx-$sz}] [expr {$cy}]
          glVertex2f [expr {$cx}] [expr {$cy+$sz}]
          glVertex2f [expr {$cx+$sz}] [expr {$cy}]
          glVertex2f [expr {$cx}] [expr {$cy-$sz}]
        glEnd
      }

      AbstractCheckbox::draw
    }
  }

  # A Slider allows the user to adjust a value in an
  # analogue way. The widget gains focus if the user
  # activates its accelerator. When focussed, it responds
  # to the left and right arrows to decrement/increment
  # the value, home to set it to the minimum, end to set
  # it to the maximum, and keys 1-0 (keyboard order, 0=10)
  # to set it to increments of 11% (1=0%, 2=11% ... 0=100%).
  # Enter, tab, and escape terminate focus, as do the number
  # keys after affecting the setting.
  # Left-clicking in the widget does not focus it, but immediately
  # moves the slider to the location indicated by the mouse, if
  # the cursor is over the actual non-label portion. Motion when
  # the left button is down has the same effect.
  #
  # The creator provides ways to access the original value,
  # to save into the original value, as well as a possible
  # hook to run upon value adjustment. The value may be an
  # integer or a float. Code may be given to present the current
  # value to the user as a label. This label must have a fixed
  # width, also specified at creation time.
  #
  # The slider is actually implemented as a BorderContainer that
  # contains special subwidgets.
  class Slider {
    inherit BorderContainer

    # Datatype is int or float
    constructor {name datatype loader saver min max
                 {increment 1} {adjustAction {}}
                 {valueFormatter {}} {valueLabelWidth -1}} {
      BorderContainer::constructor 0.01
    } {
      if {[string length $valueFormatter]} {
        if {$valueLabelWidth <= 0} {
          error "No value or bad value passed for valueLabelWidth"
        }
        set valueLabel [new ::gui::SliderFixedWidthLabel $this $valueFormatter $valueLabelWidth]
      } else {
        set valueLabel ""
      }
      set impl [new ::gui::SliderImpl $datatype $loader $saver $min $max $increment $adjustAction $valueLabel]
      set labl [new ::gui::ActivatorLabel $name $impl]

      setElt left $labl
      setElt centre $impl
      if {[string length $valueLabel]} {setElt right $valueLabel}
    }
    method isFocused {} {
      # For performance, only test the only thing that can be
      $centre isFocused
    }

    method setValue val {
      $centre setValue $val
    }

    method getValue {} {
      $centre getValue
    }
  }

  # This class was originally slider-specific (as SliderActivatorLabel),
  # but has since been generalized to work with anything with a gainFocus method
  class ActivatorLabel {
    inherit Label
    variable sliderImpl

    constructor {name impl} {
      Label::constructor $name left
    } {
      set sliderImpl $impl
    }

    method setAccelerator p {
      # Bypass Label's deactivation
      AWidget::setAccelerator $p
    }

    method action {} {
      $sliderImpl gainFocus
    }

    # Prevent clicking from focusing
    method button {evt x y} {}

    method draw {} {
      # If focussed, become all white
      if {[$sliderImpl isFocused]} {
        set old $displayName
        set displayName "\a\[(white)$old\a\]"
        Label::draw
        set displayName $old
      } else { Label::draw }
    }
  }

  class SliderFixedWidthLabel {
    inherit Label
    variable width
    variable parent
    variable formatter

    constructor {par fmt w} {
      Label::constructor "" right
    } {
      set width $w
      set parent $par
      set formatter $fmt
    }

    method draw {} {
      # Draw in all white if focused
      if {[$parent isFocused]} {
        set old $displayName
        set displayName "\a\[(white)$old\a\]"
        Label::draw
        set displayName $old
      } else { Label::draw }
    }

    method refresh val {
      setText [eval "$formatter $val"]
    }

    method minWidth {} { return $width }
  }

  class SliderImpl {
    inherit AWidget
    variable focus
    variable justGotFocus
    variable saver
    variable loader
    variable onChange
    variable valueLabel
    variable value
    variable min
    variable max
    variable datatype
    variable increment
    variable mouseOver

    constructor {dt ld sv mn mx inc aa vl} {
      AWidget::constructor
    } {
      set focus no
      set saver $sv
      set loader $ld
      set onChange $aa
      set valueLabel $vl
      set datatype $dt
      set increment $inc
      set min $mn
      set max $mx
      set mouseOver no
      set justGotFocus no

      if {$datatype != "int" && $datatype != "float"} {
        error "Bad datatype for Slider: $datatype"
      }

      revert
    }

    method isFocused {} { return $focus }
    method gainFocus {} { set focus yes; set justGotFocus yes }

    method setValue val {
      if {$datatype == "int"} {set val [expr {int($val+0.5)}]}
      if {$val < $min} {set val $min}
      if {$val > $max} {set val $max}
      set value $val
      if {[string length $valueLabel]} {$valueLabel refresh $value}
      if {[string length $onChange]} {uplevel #0 "$onChange $value"}
    }

    method getValue {} {
      return $value
    }

    method revert {} {
      setValue [uplevel #0 $loader]
    }

    method save {} {
      uplevel #0 "$saver $value"
    }

    # We aren't responsible for this, our ActivatorLabel is
    method setAccelerator p { return $p }

    method draw {} {
      set justGotFocus no
      set position [expr {double($right-$left)*double($value-$min)/double($max-$min)+$left}]

      set cy [expr {($top+$bottom)/2.0}]
      set triangleH 0.01
      set triangleW 0.005


      # Draw a base line, std if not focused, white if focused, with
      # a triangular marker, std if !mouseOver, spec if mouseOver
      if {$focus} {
        ::gui::colourWhit
      } else {
        ::gui::colourStd
      }
      glBegin GL_LINES
        glVertex2f $left $cy
        glVertex2f $right $cy
      glEnd
      if {$focus || $mouseOver} {
        ::gui::colourSpec
      } else {
        ::gui::colourStd
      }
      glBegin GL_TRIANGLES
        glVertex2f [expr {$position-$triangleW}] [expr {$cy+$triangleH}]
        glVertex2f [expr {$position+$triangleW}] [expr {$cy+$triangleH}]
        glColor4f 0 0 0 0
        glVertex2f $position [expr {$cy-$triangleH}]
      glEnd
    }

    method minWidth {} {
      # Just return something reasonable
      return 0.05
    }

    method setValueByKNumber num {
      switch $num {
        1 {set % 00}
        2 {set % 11}
        3 {set % 22}
        4 {set % 33}
        5 {set % 44}
        6 {set % 55}
        7 {set % 66}
        8 {set % 77}
        9 {set % 88}
        0 {set % 100}
      }
      setValue [expr {double($max-$min)*${%}/100.0+$min}]
    }

    method keyboard evt {
      if {$focus && !$justGotFocus} {
        switch -glob $evt {
          DOWN:MACS:k_escape {
            # EMACS -- Easter egg
            setValue [expr {$min+rand()*($max-$min)}]
          }
          DOWN:????:k_escape -
          DOWN:????:k_enter -
          DOWN:????:k_tab {set focus no}
          DOWN:????:k_[0123456789] {
            setValueByKNumber [string index $evt [expr {[string length $evt]-1}]]
            set focus no
          }
          DOWN:???-:k_left { setValue [expr {$value-$increment}] }
          DOWN:???-:k_right { setValue [expr {$value+$increment}] }
          DOWN:???S:k_left { setValue [expr {$value-$increment*5}] }
          DOWN:???S:k_right { setValue [expr {$value+$increment*5}] }
          DOWN:????:k_home { setValue $min }
          DOWN:????:k_end { setValue $max }
        }
      }
    }

    method button {evt x y} {
      if {$focus && ![contains $x $y]} {set focus no}
      if {![contains $x $y]} return
      switch -glob $evt {
        DOWN:* {set ::gui::cursorLockY $::cursorY; motion-common $x $y L----}
        UP:*   {set ::gui::cursorLockY -1}
      }
    }

    method motion {x y states} {
      if {[contains $x $y]
      ||  [contains [expr {$x-0.05}] $y]
      ||  [contains [expr {$x+0.05}] $y]} {
        motion-common $x $y $states
      }
    }

    method motion-common {x y states} {
      if {[string match L???? $states]} {
        setValue [expr \
        {max($min,min($max,($x-$left)/double($right-$left)*($max-$min)+$min))}]
      }
    }
  }

  # A SpiderGraph (I don't know what the correct name is) plots values radially,
  # relative to a given "normal" value for each radius. Anything exactly equal to
  # the normal for a radius has half the length of that radius. The radius of any
  # other value is given by (where R=total radius, v=value, n=normal)
  #   R*(1-0.5*n/v)   When v>n
  #   R*(v/n/2)       When v<n
  # To the right of the graph is a label. Without interaction, it highlights and
  # shows the name of a particular radius, cycling once every two seconds. When
  # the mouse is over the graph, it displays the name of the radius closest to
  # the mouse cursor.
  # The SpiderDatum class holds the data for each radius of a SpiderGraph.
  class SpiderDatum {
    variable name
    variable normal
    public variable value

    constructor {nam norm {val 0}} {
      set name $nam
      set normal $norm
      set value $val
    }

    method getName {} {return $name}
    method getNorm {} {return $normal}
    method getAdjustedValue {} {
      if {[catch {
        set v [expr {$value > $normal? 1.0-0.5*$normal/$value : $value/$normal/2.0}]
      }]} {
        error "expr {$value > $normal? 1.0-0.5*$normal/$value : $value/$normal/2.0}"
      }
      return $v
    }

    method fqn {} {return $this}
  }
  class SpiderGraph {
    inherit AWidget

    variable data
    variable mouseOver
    variable currentDisplay
    variable timeUntilDisplaySwitch
    variable radius

    constructor dat {
      AWidget::constructor
    } {
      set data $dat
      set mouseOver no
      set currentDisplay 0
      set timeUntilDisplaySwitch 2000
    }

    destructor {
      foreach datum $data { delete object $datum }
    }

    method minHeight {} {
      expr {[$::gui::font getHeight]*2.5}
    }
    method minWidth {} {
      set maxLabelWidth 0
      foreach datum $data {
        set w [$::gui::font width "M[$datum getName]"]
        if {$w > $maxLabelWidth} {
          set maxLabelWidth $w
        }
      }

      expr {[minHeight]+$maxLabelWidth}
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set radius [expr {($t-$b)/2.0}]
    }

    method motion {x y buttons} {
      set cy [expr {($bottom+$top)/2.0}]
      if {$x >= $left && $x <= ($left+2*$radius)
      &&  $y >= $cy-$radius && $y <= $cy+$radius} {
        set mouseOver yes

        set cx [expr {$left+$radius}]
        set rx [expr {$x-$cx}]
        set ry [expr {$y-$cy}]
        set angle [expr {atan2($ry,$rx)}]
        set pi [expr {acos(-1)}]
        # Modify angle from -pi..pi to 0..2*pi
        if {$angle < 0} {set angle [expr {$angle+2*$pi}]}
        set anglePerDatum [expr {2*$pi/[llength $data]}]
        # By rounding angle/anglePerDatum, we get the nearest
        # datum, EXCEPT in the case of the upper half of the
        # segment approaching 2*pi, in which case we actually
        # use datum 0
        set ix [expr {round($angle/$anglePerDatum)}]
        if {$ix == [llength $data]} {set ix 0}

        # Yay
        set currentDisplay $ix
        set timeUntilDisplaySwitch 0
      } else {
        set mouseOver no
      }
    }

    method draw {} {
      # Update first
      global applicationUpdateTime
      if {!$mouseOver} {
        set timeUntilDisplaySwitch [expr {$timeUntilDisplaySwitch-$applicationUpdateTime}]
        if {$timeUntilDisplaySwitch < 0} {
          set timeUntilDisplaySwitch 2000
          set currentDisplay [expr {($currentDisplay+1)%[llength $data]}]
        }
      }

      set cx [expr {$left+$radius}]
      set cy [expr {($top+$bottom)/2.0}]
      set pi [expr {acos(-1)}]
      set anglePerDatum [expr {2*$pi/[llength $data]}]

      # Draw the graph itself
      # Since it will be concave much of the time, we must use
      # a triangle fan instead of a simple polygon
      ::gui::colourSpec
      glBegin GL_TRIANGLE_FAN
      glVertex2f $cx $cy
      # Loop through the data; we must draw the first datum as the
      # last to complete the figure
      for {set i 0} {$i <= [llength $data]} {incr i} {
        set datum [lindex $data [expr {$i%[llength $data]}]]
        set amt [$datum getAdjustedValue]
        set angle [expr {$anglePerDatum*$i}]
        # Colour towards spec as we approach zero, and toward
        # dang as we approach one, with std at the middle.
        if {$amt < 0.5} {
          ::gui::blendedColours special [expr {1.0-$amt*2}] standard [expr {$amt*2}]
        } else {
          ::gui::blendedColours danger [expr {($amt-0.5)*2}] standard [expr {1.0-($amt-0.5)*2}]
        }
        glVertex2f [expr {$cx+$radius*$amt*cos($angle)}] \
                   [expr {$cy+$radius*$amt*sin($angle)}]
      }
      glEnd

      # Highlight current datum
      ::gui::colourWhit
      glBegin GL_LINES
      glVertex2f $cx $cy
      set angle [expr {$anglePerDatum*$currentDisplay}]
      glVertex2f [expr {$cx+$radius*cos($angle)}] [expr {$cy+$radius*sin($angle)}]
      glEnd

      # Draw text of current item
      ::gui::colourStd
      $::gui::font preDraw
      $::gui::font drawStr [[lindex $data $currentDisplay] getName] \
                           [expr {$cx+$radius+[$::gui::font charWidth M ]}] \
                           [expr {$cy-[$gui::font getHeight]/2.0}]
      $::gui::font postDraw
    }
  }

  # A TextField allows the user to supply an arbitrary
  # single line of text. An ActivatorLabel is used at
  # left, and TextFieldImpl in the centre. Using the
  # mouse to focus the text field and locate the cursor
  # is supported; key bindings are as follows:
  #   left  Move cursor one character left
  #   right Move cursor one character right
  #   home  Move cursor to position zero
  #   end   Move cursor past last character
  #   back  Delete character to left of cursor
  #   del   Delete character to right of cursor
  #   tab   Defocus
  #   esc   Defocus
  #   enter Action and defocus
  # An action script (for when the user presses enter) and
  # a validation script (which checks whether a new value
  # of the text box) may be supplied, as well as normal
  # load/save scripts.
  #
  # A TextField may be configured to draw *s instead of
  # the actual text with the setObscured method.
  class TextField {
    inherit BorderContainer

    constructor {name inittext {load {}} {save {}}
                 {actionScript {}} {validateScript {}}} {
      BorderContainer::constructor 0.01
    } {
      set impl [new ::gui::TextFieldImpl $inittext $load $save $actionScript $validateScript]
      set labl [new ::gui::ActivatorLabel $name $impl]
      setElt left $labl
      setElt centre $impl
    }

    method isFocused {} {
      $centre isFocused
    }

    # Allow retrieving and setting the contents of the field
    method getText {} {
      $centre getText
    }

    method setText {t} {
      $centre setText $t
    }

    method setObscured {t} {
      $centre setObscured $t
    }

    method gainFocus args {
      $centre gainFocus
    }

    # Sets whether the TextField may be focused
    method setEnabled enabled {
      $centre setEnabled $enabled
    }
  }

  class TextFieldImpl {
    inherit AWidget

    # The textual contents
    variable contents
    # The current cursor position
    # (actually, the character immediately
    # following the cursor)
    variable cursor
    # If the text is wider than the box,
    # we scroll by characters. This is
    # the index of the first character being
    # displayed
    variable scroll
    # Whether we are focused
    variable focus
    # Set to true if we were just focused by
    # the keyboard (so we know to ignore the
    # first key event)
    variable justFocused

    variable textBase
    variable textTop

    #Scripts
    variable loader
    variable saver
    variable actionScript
    variable validator

    # True if we show *s instead of characters
    variable obscured

    # Whether to allow focus
    variable enabled

    constructor {initContents ld sv as vd} {
      AWidget::constructor
    } {
      set contents $initContents
      set cursor 0
      set scroll 0
      set focus no
      set justFocused no
      set loader $ld
      set saver $sv
      set actionScript $as
      set validator $vd
      set obscured no
      set enabled yes
      revert
    }

    method isFocused {} {
      return $focus
    }

    # Called only by the ActivatorLabel by keyboard activity
    method gainFocus {} {
      if {!$enabled} return
      set focus yes
      set justFocused yes
      moveCursor [string length $contents]
    }

    method loseFocus {} {
      set focus no
      set scroll 0
    }

    method setObscured t {
      set obscured $t
    }

    # We don't need to override action, because we handle
    # mouse clicks ourselves anyway

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      # Set the text locations
      set textBase [expr {($t+$b)/2.0 - [$::gui::font getHeight]/2.0}]
      set textTop [expr {[$::gui::font getHeight]+$textBase}]
    }

    method getVText {} {
      set ret $contents
      if {$obscured} {
        set ret [string repeat * [string length $ret]]
      }
      return $ret
    }

    method draw {} {
      set justFocused no

      global ::gui::font
      # Draw a simple frame first
      # White if focused, std otherwise
      if {$focus} ::gui::colourSpec else ::gui::colourStd
      glBegin GL_LINE_STRIP
        glVertex2f $left $textTop
        glVertex2f $right $textTop
        glVertex2f $right $textBase
        glVertex2f $left $textBase
        glVertex2f $left $textTop
      glEnd

      set txt [getVText]

      # Find the appropriate length of string to draw
      set strend [string length $txt]
      while {[$font width [string range $txt $scroll $strend]] > ($right-$left)} \
        {incr strend -1}
      # Draw text
      ::gui::colourStd
      $font preDraw
      set textToDraw [string range $txt $scroll $strend]
      $font drawStr $textToDraw $left $textBase
      $font postDraw

      # Draw blinking cursor if focused
      if {$focus && ($::gameClock % 500 > 250)} {
        set cursorX [$font width [string range $txt $scroll [expr {$cursor-1}]]]
        set cursorX [expr {$cursorX + $left}]
        ::gui::colourWhit
        glBegin GL_QUADS
          glVertex2f $cursorX $textBase
          glVertex2f $cursorX $textTop
          glVertex2f [expr {$cursorX+0.005}] $textTop
          glVertex2f [expr {$cursorX+0.005}] $textBase
        glEnd
      }
    }

    method revert {} {
      if {[string length $loader]} {
        set contents [namespace eval :: $loader]
      }
    }

    method save {} {
      if {[string length $saver]} {
        # Making a singleton list will properly brace/escape
        # everything for reparsing
        namespace eval :: "$saver [list $contents]"
      }
    }

    method keyboard evt {
      if {!$focus || $justFocused} return
      switch -glob $evt {
        DOWN:????:k_enter {
          if {[string length $actionScript]} {
            namespace eval :: "$actionScript [list $contents]"
          }
          loseFocus
        }
        DOWN:????:k_left {
          if {$cursor > 0} {moveCursor [expr {$cursor-1}]}
        }
        DOWN:????:k_right {
          if {$cursor < [string length $contents]} {
            moveCursor [expr {$cursor+1}]
          }
        }
        DOWN:????:k_home { moveCursor 0 }
        DOWN:????:k_end  { moveCursor [string length $contents] }
        DOWN:????:k_backspace {
          if {$cursor > 0} {
            set prefix [string range $contents 0 [expr {$cursor-2}]]
            set postfix [string range $contents $cursor [string length $contents]]
            set contents $prefix$postfix
            moveCursor [expr {$cursor-1}]
          }
        }
        DOWN:????:k_delete {
          if {$cursor < [string length $contents]} {
            set prefix [string range $contents 0 [expr {$cursor-1}]]
            set postfix [string range $contents [expr {$cursor+1}] [string length $contents]]
            set contents $prefix$postfix
          }
        }
        DOWN:????:k_tab -
        DOWN:????:k_escape {loseFocus}
      }
    }

    method character ch {
      if {$justFocused} return
      if {![isFocused]} return
      if {[toUnicode $ch] >= 0x20 && [toUnicode $ch] != 0x7F} {
        # Insert
        set prefix [string range $contents 0 [expr {$cursor-1}]]
        set postfix [string range $contents $cursor [string length $contents]]
        set newContents $prefix$ch$postfix
        # Only update contents if valid
        if {[string length $validator]==0
        ||  [namespace eval :: "$validator [list $newContents]"]} {
          set contents $newContents
          moveCursor [expr {$cursor+1}]
        }
      }
    }

    method button {evt x y} {
      if {[contains $x $y]} {
        if {$evt == "DOWN:mb_left"} {
          set focus yes
          set x [expr {$x-$left}]
          set cursor $scroll
          # Find the first character that extends beyond
          # the click location
          while {[$::gui::font width \
                  [string range [getVText] $scroll $cursor]
                ] < $x && $cursor < [string length $contents]} {incr cursor}
        }
      } else loseFocus
    }

    method moveCursor ix {
      # Move the cursor and adjust scroll appropriately
      set cursor $ix
      # If the cursor is before scroll, just move scroll to that
      if {$cursor < $scroll} {
        set scroll [expr {max(0, $cursor-1)}]
      } else {
        #Make sure everything fits
        while {[$::gui::font width [string range $contents $scroll $cursor]] > ($right-$left)} {
          incr scroll
        }
      }
    }

    method getText {} {return $contents}
    method setText txt {set contents $txt}
    method setEnabled enab {
      set enabled $enab
      if {!$enabled} loseFocus
    }
  }

  # A Scrollbar allows the user to choose an integer from a
  # range, while also showing how much is visible. A Scrollbar
  # can be either vertical (top-to-bottom) or horizontal
  # (right-to-left). The widget interfaces with another that
  # provides the following methods:
  #   method getIndex {} -> int, returns current value
  #   method getView {} -> int, returns visible amount
  #   method getMax {} -> int, returns the maximum value of (curr+view)
  #   method setIndex int -> void, sets the current scroll
  # The minimum value is assumed to be zero.
  # A vertical scrollbar will also respond to mouse wheel events
  # over its interface.
  # It is up to the interface to make sure that input indices are valid
  set scrollBarThickness [$font width M]
  class Scrollbar {
    inherit AWidget
    variable isVertical
    variable interface

    constructor {iface {isVert yes}} {
      AWidget::constructor
    } {
      set isVertical $isVert
      set interface $iface
    }

    method minWidth {} {
      return $::gui::scrollBarThickness
    }
    method minHeight {} {
      return $::gui::scrollBarThickness
    }

    method draw {} {
      # First, determine percent bounds for the
      # main handle and the view extension
      set max  [$interface getMax]
      set view [$interface getView]
      set ix   [$interface getIndex]
      set ixp [expr {$ix/1.0/$max}]
      set viewp [expr {$view/1.0/$max}]

      # We draw the base of the scroll bar in std, like this:
      # <-------------------->
      # We then use spec to draw a fading triangle going to
      # the bottom/right, indicating the view size (ie, going
      # from ixp to viewp); then, a line perpendicular to the
      # bar in white indicating ixp
      ::gui::colourStd
      if {$isVertical} {
        set ixp [expr {1-$ixp}]
        set cx [expr {($right+$left)/2}]
        set lt [expr {$top-$::gui::scrollBarThickness}]
        set ub [expr {$bottom+$::gui::scrollBarThickness}]
        set viewy [expr {($top-$bottom-2*$::gui::scrollBarThickness)*($ixp-$viewp)
                        + $bottom+$::gui::scrollBarThickness}]
        set ixy   [expr {($top-$bottom-2*$::gui::scrollBarThickness)*($ixp)
                        + $bottom+$::gui::scrollBarThickness}]
        glBegin GL_LINES
        glVertex2f $cx $top
        glVertex2f $cx $bottom
        glVertex2f $cx $top
        glVertex2f $left $lt
        glVertex2f $cx $top
        glVertex2f $right $lt
        glVertex2f $cx $bottom
        glVertex2f $left $ub
        glVertex2f $cx $bottom
        glVertex2f $right $ub
        glEnd

        ::gui::colourSpec
        glBegin GL_TRIANGLES
        glVertex2f $left $ixy
        glVertex2f $right $ixy
        ::gui::colourSpec 0.5
        glVertex2f $cx $viewy
        glEnd

        ::gui::colourWhit
        glBegin GL_LINES
        glVertex2f $left $ixy
        glVertex2f $right $ixy
        glEnd
      } else {
        set cy [expr {($top+$bottom)/2}]
        set rl [expr {$left + $::gui::scrollBarThickness}]
        set lr [expr {$right - $::gui::scrollBarThickness}]
        set ixx   [expr {($right-$left-2*$::gui::scrollBarThickness)*$ixp
                        + $left+$::gui::scrollBarThickness}]
        set viewx [expr {($right-$left-2*$::gui::scrollBarThickness)*($ixp+$viewp)
                        + $left+$::gui::scrollBarThickness}]
        glBegin GL_LINES
        glVertex2f $left  $cy
        glVertex2f $right $cy
        glVertex2f $left  $cy
        glVertex2f $rl    $top
        glVertex2f $left  $cy
        glVertex2f $rl    $bottom
        glVertex2f $right $cy
        glVertex2f $lr    $top
        glVertex2f $right $cy
        glVertex2f $lr    $bottom
        glEnd

        ::gui::colourSpec
        glBegin GL_TRIANGLES
        glVertex2f $ixx $top
        glVertex2f $ixx $bottom
        ::gui::colourSpec 0.5
        glVertex2f $viewx $cy
        glEnd

        ::gui::colourWhit
        glBegin GL_LINES
        glVertex2f $ixx $top
        glVertex2f $ixx $bottom
        glEnd
      }
    }

    method button {evt x y} {
      switch -glob $evt {
        DOWN:mb_left -
        DOWN:mb_middle {
          if {[contains $x $y]} {
            set ::gui::cursorLockX $::cursorX
            # Jump to that position
            if {$isVertical} {
              set off [expr {$y-$bottom-$::gui::scrollBarThickness}]
              set percent [expr {1-$off/($top-$bottom-2*$::gui::scrollBarThickness)}]
              $interface setIndex [expr {int([$interface getMax]*$percent)}]
            } else {
              set off [expr {$x-$left-$::gui::scrollBarThickness}]
              set percent [expr {$off/($right-$left-2*$::gui::scrollBarThickness)}]
              $interface setIndex [expr {int([$interface getMax]*$percent})]
            }
          }
        }
        UP:mb_left -
        UP:mb_middle {
          set ::gui::cursorLockX -1
        }
        DOWN:mb_wup {
          if {[contains $x $y] || ($isVertical && [$interface contains $x $y])} {
            $interface setIndex [expr {[$interface getIndex]-3}]
          }
        }
        DOWN:mb_wdown {
          if {[contains $x $y] || ($isVertical && [$interface contains $x $y])} {
            $interface setIndex [expr {[$interface getIndex]+3}]
          }
        }
      }
    }

    # Respond to dragging
    method motion {x y states} {
      if {![contains $x $y]} return
      switch -glob -- $states {
        L???? -
        ?M--- {
          button DOWN:mb_left $x $y
        }
      }
    }
  }

  # A List provides the user with a list of textual options.
  # The user can select items by focusing the list and using
  # the arrow keys, or by clicking on the item (which does
  # not focus the list). A list may allow the user to select
  # more than one item; this is done in any of the following
  # ways:
  # + Holding control while clicking will toggle the clicked
  #   item without deselecting anything else
  # + Holding shift while clicking will select everything
  #   between the previously-toggled item and the clicked one,
  #   inclusive
  # + Holding control when pressing an arrow key will not deselect
  #   the current selection
  # + Holding shift when pressing an arrow key will select
  #   everything between the previously-toggled item and
  #   the new item, inclusive
  # As with most components, loader and saver code, as well as
  # code for change notification
  class List {
    inherit BorderContainer

    constructor {name {items {}} {multiselect no}
                 {onChange {}} {loader {}} {saver {}}} {
      BorderContainer::constructor 0.01
    } {
      set impl [new ::gui::ListImpl $items $multiselect $onChange $loader $saver]
      set labl [new ::gui::ActivatorLabel $name $impl]
      set scroll [new ::gui::Scrollbar $impl]
      setElt top $labl
      setElt centre $impl
      setElt right $scroll
    }

    method isFocused {} {
      $centre isFocused
    }

    # Returns an index or list of indices
    method getSelection {} {
      $centre getSelection
    }

    # Sets the selection to a index or a list of indices
    method setSelection indices {
      $centre setSelection $indices
    }

    method getItems {} {
      $centre getItems
    }

    # Sets the items in the List, and clears the selection
    method setItems items {
      $centre setItems $items
    }
  }

  class ListImpl {
    inherit AWidget

    # Whether we use multiple selection
    variable multiselect
    # The list of items
    variable items
    # Whether each item is selected
    variable selected

    # Whether focused, and which item is currently in focus
    variable focus
    variable focusItem
    # The previously toggled item
    variable togglePrevious

    # The current scroll
    variable scroll
    # The max number of items visible ((top-bottom)/font_height)
    variable view

    variable loader
    variable saver
    variable onChange

    constructor {ims ms oc ld sv} {
      AWidget::constructor
    } {
      set multiselect $ms
      set items $ims
      resetSelection
      set focus no
      set focusItem 0
      set togglePrevious 0
      set scroll 0
      set loader $ld
      set saver $sv
      set onChange $oc
      set view 1 ;# In case not sized yet
    }

    method resetSelection {} {
      set selected {}
      foreach item $items {
        lappend selected no
      }
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set view [expr {int(($t-$b)/[$::gui::font getHeight])}]
      # Make sure scrolling is valid
      setIndex $scroll
    }

    method isFocused {} {
      return $focus
    }

    method gainFocus {} {
      set focus yes
    }

    method loseFocus {} {
      set focus no
    }

    method getSelection {} {
      set lst {}
      for {set i 0} {$i < [llength $selected]} {incr i} {
        if {[lindex $selected $i]} {
          lappend lst $i
        }
      }
      return $lst
    }

    method setSelection sel {
      resetSelection
      foreach i $sel {
        set selected [lreplace $selected $i $i yes]
      }
    }

    method getItems {} {
      return $items
    }

    method setItems ims {
      if {$ims != $items} {
        set items $ims
        resetSelection
        # Make sure scrolling is still valid
        setIndex $scroll
        set focusItem 0
        set previousToggle 0
      }
    }

    method draw {} {
      global ::gui::font
      # First, a frame around the list, in white if focused, std otherwise
      # We draw each item in std if deselected, or white with spec
      # background if selected. If focused and this is the current focus item,
      # draw a white line above and below the text
      if {$focus} {
        ::gui::colourWhit
      } else {
        ::gui::colourStd
      }
      # Frame
      glBegin GL_LINE_STRIP
        glVertex2f $left $top
        glVertex2f $right $top
        glVertex2f $right $bottom
        glVertex2f $left $bottom
        glVertex2f $left $top
      glEnd

      # Draw each item
      set h [$font getHeight]
      set y [expr {$top-$h}]
      for {set i $scroll} {$i < $scroll+$view && $i < [llength $items]} {incr i} {
        set item [lindex $items $i]
        # Background if selected
        if {[lindex $selected $i]} {
          ::gui::colourSpec
          glBegin GL_QUADS
            glVertex2f $left $y
            glVertex2f $right $y
            glVertex2f $right [expr {$y+$h}]
            glVertex2f $left [expr {$y+$h}]
          glEnd
        }

        #Lines if focused and curr
        if {$focus && ($i == $focusItem)} {
          ::gui::colourWhit
          glBegin GL_LINES
            glVertex2f $left $y
            glVertex2f $right $y
            glVertex2f $right [expr {$y+$h}]
            glVertex2f $left [expr {$y+$h}]
          glEnd
        }

        #Text
        if {[lindex $selected $i]} {
          ::gui::colourWhit
        } else {
          ::gui::colourStd
        }
        $font preDraw
        set end [string length $item]
        # Truncate until it fits
        while {[$font width [string range $item 0 $end]] > $right-$left} { incr end -1 }
        $font drawStr [string range $item 0 $end] $left $y
        $font postDraw

        # Move to next line
        set y [expr {$y-$h}]
      }
    }

    method minHeight {} {
      expr {[$::gui::font getHeight]*3}
    }

    # Keyboard:
    #   up    Move cursor up, change selection, set previousToggle
    #   down  Move cursor down
    #   pgdn  Move cursor down by view
    #   pgup  Move cursor up by view
    #   home  Move to item 0
    #   end   Move to last item
    #   C-dir Same as dir, but don't modify selection or previousToggle
    #   S-dir Same as dir, but don't clear selection
    #   space Clear selection and select current, set previousToggle
    #   C-space Toggle current, set previousToggle
    #   S-space Select all fromm previoaus
    #   tab   Defocus
    #   enter Defocus
    #   esc   Defocus
    #   MACS-esc Randomize selection
    method keyboard evt {
      if {!$focus} return
      switch -glob $evt {
        DOWN:----:k_up       { moveCursorRel -1 yes yes yes }
        DOWN:----:k_down     { moveCursorRel +1 yes yes yes }
        DOWN:----:k_pageup   { moveCursorRel -$view yes yes yes }
        DOWN:----:k_pagedown { moveCursorRel +$view yes yes yes }
        DOWN:----:k_home     { moveCursor 0 yes yes yes }
        DOWN:----:k_end      { moveCursor [expr {[llength $items]-1}] yes yes yes }
        DOWN:--C-:k_up       { moveCursorRel -1 no no no }
        DOWN:--C-:k_down     { moveCursorRel +1 no no no }
        DOWN:--C-:k_pageup   { moveCursorRel -$view no no no }
        DOWN:--C-:k_pagedown { moveCursorRel +$view no no no }
        DOWN:--C-:k_home     { moveCursor 0 no no no }
        DOWN:--C-:k_end      { moveCursor [expr {[llength $items]-1}] no no no }
        DOWN:---S:k_up       { moveCursorRel -1 yes yes no yes }
        DOWN:---S:k_down     { moveCursorRel +1 yes yes no yes }
        DOWN:---S:k_pageup   { moveCursorRel -$view yes yes no yes }
        DOWN:---S:k_pagedown { moveCursorRel +$view yes yes no yes }
        DOWN:---S:k_home     { moveCursor 0 yes yes no yes }
        DOWN:---S:k_end      { moveCursor [expr {[llength $items]-1}] yes yes no yes }
        DOWN:----:k_space    { resetSelection; toggleCurrent }
        DOWN:--C-:k_space    { toggleCurrent }
        DOWN:--CS:k_space -
        DOWN:---S:k_space    { selectThroughCurrent }
        DOWN:????:k_enter -
        DOWN:????:k_escape -
        DOWN:????:k_tab      { loseFocus }
      }
    }

    method button {evt x y} {
      # Only respond to left-click down
      if {$evt != "DOWN:mb_left"} return
      if {![contains $x $y]} {
        loseFocus
        return
      }

      # Determine which item it is over
      set item [expr {$scroll+int(($top-$y)/[$::gui::font getHeight])}]
      # There is a possibility of being below the bottom displayed;
      # we need to ignore that case
      if {$item >= $scroll+$view} return
      if {$item >= [llength $items]} return

      # Set the current to that item, then act as if
      # we pressed spacebar with the current mods
      set focusItem $item
      set oldFocus $focus
      set focus yes ;# So keyboard won't ignore our fake event
      global currentKBMods
      keyboard DOWN:$currentKBMods:k_space
      set focus $oldFocus
    }

    method moveCursor {where {select no} {setToggle no} {clearSelection no} {selectThrough no}} {
      set where [expr {max(0, min([llength $items]-1, $where))}]
      set focusItem $where
      if {$select} {
        if {$clearSelection} {
          resetSelection
        }
        if {$selectThrough} {
          selectThroughCurrent
        } else {
          toggleCurrent
        }
      }

      # Make sure on screen
      if {$focusItem < $scroll} {
        set scroll $focusItem
      } elseif {$scroll+$view <= $focusItem} {
        set scroll [expr {$focusItem-$view+1}]
      }
    }

    method moveCursorRel {where args} {
      moveCursor [expr {$focusItem+$where}] {*}$args
    }

    method toggleCurrent {} {
      # Handle empty lists correctly
      if {0==[llength $selected]} return

      if {!$multiselect} resetSelection
      set selected [
        lreplace $selected $focusItem $focusItem [expr {
          ![lindex $selected $focusItem]
        }]
      ]
      set togglePrevious $focusItem
      namespace eval :: $onChange
    }

    method selectThroughCurrent {} {
      # Handle empty lists correctly
      if {0==[llength $selected]} return

      if {!$multiselect} toggleCurrent \
      else {
        set lower [expr {min($togglePrevious, $focusItem)}]
        set upper [expr {max($togglePrevious, $focusItem)}]
        for {set i $lower} {$i <= $upper} {incr i} {
          set selected [lreplace $selected $i $i yes]
        }
        set togglePrevious $focusItem
        namespace eval :: $onChange
      }
    }

    # Scrollbar interface
    method getIndex {} {
      return $scroll
    }

    method setIndex ix {
      set scroll [expr {max(0, min([llength $items]-$view, $ix))}]
    }

    method getView {} {
      expr {min($view, max(1,[llength $items]))}
    }

    method getMax {} {
      expr {max([llength $items],1)}
    }
  }

  # The ProgressBar is a simple component that uses a horizontal
  # bar to represent progress, which is a range from 0 to 1.
  class ProgressBar {
    inherit AWidget

    variable progress

    constructor {{initprogress 0}} {
      AWidget::constructor
    } {
      set progress $initprogress
    }

    method getProgress {} {
      return $progress
    }

    method setProgress p {
      set progress $p
    }

    method minHeight {} {
      $::gui::font getHeight
    }

    method draw {} {
      # Draw progress
      set rght [expr {$left + $progress*($right-$left)}]
      glBegin GL_QUADS
      ::gui::colourStd
      glVertex $left $top
      glVertex $left $bottom
      ::gui::colourSpec
      glVertex $rght $bottom
      glVertex $rght $top
      glEnd

      # Draw frame on top
      ::gui::colourStd
      glBegin GL_LINE_LOOP
      glVertex $left  $bottom
      glVertex $right $bottom
      glVertex $right $top
      glVertex $left  $top
      glEnd
    }
  }

  # The ::gui::SquareIcon is a front-end to ::SquareIcon, wrapping
  # it in an AWidget.
  # For sizing, it defaults to no minimum. However, it can be attached
  # to another component on one axis, whose minima it will
  # duplicate across both axes (so that it is square)
  # It forwards all ::SquareIcon method calls to the internal instance
  # of that class.
  class SquareIcon {
    inherit AWidget

    variable icon
    variable xmin
    variable ymin

    constructor {} {
      AWidget::constructor
    } {
      set icon [namespace eval :: new SquareIcon default]
      set xmin {}
      set ymin {}
    }

    method draw {} {
      set dim [expr {min($top-$bottom,$right-$left)}]
      glPushMatrix
      glTranslate [expr {$left + ($right-$left-$dim)/2}] \
                  [expr {$bottom + ($top-$bottom-$dim)/2}]
      glUScale $dim
      $icon draw
      glPopMatrix
    }

    # Causes the widget to report minimum dimensions equal to the
    # given widget's width. If this is a float, it is used literally
    # instead of as a widget.
    # Passing an empty argument resets minima to zero.
    method dupeMinWidth w {
      set xmin $w
      set ymin {}
    }

    # Causes the widget to report minimum dimensions equal to the
    # given widget's height. If this is a float, it is used literally
    # instead of as a widget.
    # Passing an empty argument resets the minima to zero.
    method dupeMinHeight w {
      set ymin $w
      set xmin {}
    }

    method minHeight {} {
      if {"" != $xmin} {
        if {[string is double $xmin]} {
          expr {$xmin}
        } else {
          $xmin minWidth
        }
      } elseif {"" != $ymin} {
        if {[string is double $ymin]} {
          expr {$ymin}
        } else {
          $ymin minHeight
        }
      } else {
        expr 0
      }
    }
    method minWidth {} minHeight

    foreach meth {isLoaded load save unload} {
      method $meth args "\$icon $meth {*}\$args"
    }
  }

  ## BEGIN: Abendstern-specific components

  # The ShipSpiderGraph is a common component for displaying
  # a ship's abilities.
  class ShipSpiderGraph {
    inherit SpiderGraph

    #SpiderData
    variable sdAccel ;#Acceleration
    variable sdRota  ;#Rotational acceleration
    variable sdPower ;#Total power
    variable sdFPow  ;#Free power
    variable sdCapac ;#Capacitance
    variable sdArmour;#Armour (reinforcement)
    variable sdMass  ;#Mass

    constructor {} {
      set sdAccel  [new ::gui::SpiderDatum [_ A ship_chooser acceleration] 4.5e-7]
      set sdRota   [new ::gui::SpiderDatum [_ A ship_chooser rotation] 3.5e-6]
      set sdPower  [new ::gui::SpiderDatum [_ A ship_chooser power] 15.0]
      set sdFPow   [new ::gui::SpiderDatum [_ A ship_chooser free_power] 7.5]
      set sdCapac  [new ::gui::SpiderDatum [_ A ship_chooser capacitance] 10000.0]
      set sdArmour [new ::gui::SpiderDatum [_ A ship_chooser reinforcement] 5.0]
      set sdMass   [new ::gui::SpiderDatum [_ A ship_chooser mass] 16384.0]
      SpiderGraph::constructor [list \
        $sdAccel $sdRota   $sdPower  $sdFPow \
        $sdCapac $sdArmour $sdMass]
    } {
    }

    method setShip ship {
      $sdAccel  configure -value [$ship getAcceleration]
      $sdRota   configure -value [$ship getRotationAccel]
      $sdPower  configure -value [expr {[$ship getPowerSupply]/double([$ship cellCount])}]
      $sdFPow   configure -value [expr {max(0,
        ([$ship getPowerSupply]-[$ship getPowerDrain])/double([$ship cellCount]))}]
      $sdCapac  configure -value [expr {[$ship getMaximumCapacitance]/double([$ship cellCount])}]
      #$sdFireP configure -value [...]
      $sdArmour configure -value [$ship getReinforcement]
      $sdMass   configure -value [$ship getMass]
    }
  }

  # ShipChooser allows the user to select a Ship from a libconfig
  # catalogue. The current ship, along with some info, is shown
  # in the dialogue.
  # Stats for power and capacitance are the raw values divided
  # by the number of cells in the ship.
  # Whenever the ship is changed, the provided action is called,
  # with the new index appended.
  # At the bottom is a search field; when the user types into
  # this field, only ships whose names contain that string
  # (case insensitive) will be displayed, unless no ships do,
  # in which case the current ship cannot be changed
  class ShipChooser {
    inherit BorderContainer

    variable catalogue
    variable index
    variable display
    variable ship
    variable field

    variable ssgGraph
    variable action

    variable nameLabel
    variable authorLabel
    variable searchField
    variable prevSearchText

    constructor {cat act {init 0}} {
      BorderContainer::constructor
    } {
      set catalogue $cat
      set index $init
      set action $act
      set display [new ::gui::SimpleShipDisplay]
      set field [new GameField default 1 1]
      set ship ""

      set selector [new ::gui::BorderContainer]
      set leftb  [new ::gui::Button "<<" "$this previous"]
      set rightb [new ::gui::Button ">>" "$this next"]
      $leftb setLeft
      $rightb setRight
      $selector setElt left $leftb
      $selector setElt right $rightb
      set nameLabel [new ::gui::Label "NAME HERE" centre]
      $selector setElt centre $nameLabel
      setElt top $selector

      set main [new ::gui::StackContainer]
      $main add $display
      set labelPan [new ::gui::VerticalContainer]
      set authorPan [new ::gui::HorizontalContainer]
      $authorPan add [new ::gui::Label "[_ A editor author]: " left]
      set authorLabel [new ::gui::Label "AUTHOR HERE" left]
      $authorPan add $authorLabel
      $labelPan add $authorPan
      set ssgGraph [new ::gui::ShipSpiderGraph]
      $labelPan add $ssgGraph
      $main add $labelPan
      setElt centre $main

      set searchField [new ::gui::TextField [_ A ship_chooser filter] ""]
      set prevSearchText ""
      setElt bottom $searchField

      loadShip
    }

    method reset {cat {ix 0}} {
      set catalogue $cat
      set index $ix
      loadShip
    }

    destructor {
      if {[string length $ship]} {delete object $ship}
      delete object $field
    }

    method draw {} {
      # Before drawing, check to see if the filter text excludes the
      # current ship
      if {$prevSearchText != [$searchField getText]} {
        catch {
          set shipName [$ str [getAt $index].info.name]
          set filter [$searchField getText]
          set prevSearchText $filter
          if {[string length $filter]
          &&  -1 == [string first [string tolower $filter] [string tolower $shipName]]} {
            next
          }
        }
      }

      ::gui::Container::draw
    }

    method getAt ix {
      set mp [$ str $catalogue.\[$ix\]]
      if {-1 == [string first : $mp]} {
        set mp ship:$mp
      }
      return $mp
    }

    method loadShip {} {
      if {[string length $ship]} {delete object $ship}
      if {[catch {
        set ship ""
        set mp [getAt $index]
        set ship [::loadShip $field $mp]
        $display setShip $ship

        $ship configureEngines yes no 1.0

        $ssgGraph setShip $ship

        $nameLabel setText [$ str $mp.info.name]
        if {[$ exists $mp.info.author]} {
          $authorLabel setText [$ str $mp.info.author]
        } else {
          $authorLabel setText [_ A gui unknown]
        }
      } err]} {
        # If there is no ship in the current catalogue,
        # the puts command will fail, as $mp will not be defined
        if {[catch {
          #puts "$mp: $err"
          set err "$mp: $err"
        }]} {
          set err [_ A ship_chooser no_ships]
        }
        $nameLabel setText "\a\[(danger)$err\a\]"
        $authorLabel setText "---"
        $display setShip error
      }
    }

    method getNameSafe root {
      set ret {}
      catch {
        set ret [$ str $root.info.name]
      } err
      return $ret
    }

    method previous {} {
      # If there are zero or one ships in the catalogue,
      # do nothing. Otherwise, cycle until we encounter
      # something different or hit the original index
      if {[$ length $catalogue] <= 1} return

      set origix $index
      set first yes
      while {$first ||
             ($origix != $index
           && ([$ str $catalogue.\[$origix\]] == [getAt $index]
            || ([string length [$searchField getText]]
              && -1 == [string first \
                        [string tolower [$searchField getText]] \
                        [string tolower [getNameSafe [getAt $index]]]])))} {
        set index [expr {($index-1)%[$ length $catalogue]}]
        set first no
      }
      eval "$action $index"
      loadShip
    }

    method next {} {
      # If there are zero or one ships in the catalogue,
      # do nothing. Otherwise, cycle until we encounter
      # something different or hit the original index
      if {[$ length $catalogue] <= 1} return

      set origix $index
      set first yes
      while {$first ||
             ($origix != $index
           && ([$ str $catalogue.\[$origix\]] == [getAt $index]
            || ([string length [$searchField getText]]
              && -1 == [string first \
                        [string tolower [$searchField getText]] \
                        [string tolower [getNameSafe [getAt $index]]]])))} {
        set index [expr {($index+1)%[$ length $catalogue]}]
        set first no
      }
      eval "$action $index"
      loadShip
    }
  }

  # Displays a Ship, scaled to fit within the boundaries.
  # If the ship is "error", a danger X is shown instead
  class SimpleShipDisplay {
    inherit AWidget

    variable ship

    constructor {} {AWidget::constructor} {set ship ""}

    method setShip sh {
      set ship $sh
      if {$ship != "" && $ship != "error"} {$ship teleport 0 0 0}
    }

    method draw {} {
      global ::cameraZoom
      set cameraZoom 1
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set minDim [expr {min($w,$h)}]
      set cx [expr {($right+$left)/2.0}]
      set cy [expr {($top+$bottom)/2.0}]

      if {$ship == ""} {return} \
      elseif {$ship == "error"} {
        ::gui::colourDang
        glBegin GL_LINES
          glVertex2f $left $top
          glVertex2f $right $bottom
          glVertex2f $right $top
          glVertex2f $left $bottom
        glEnd
        return
      }

      # Normal ship
      set shipDim [expr {[$ship getRadius]*2.0}]
      # Scale out, unless ship already fits
      set scale [expr {min(1.0, $minDim/$shipDim)}]

      # Draw
      glPushMatrix
      glTranslatef $cx $cy 0
      glScalef $scale $scale 1
      $ship draw
      glPopMatrix
    }
  }

  # The HangarEditor is a simple Mode that allows the user to
  # edit a hangar. It uses a simple two-paned design:
  #   Name [................................................]
  #   /-------------\                       /---------------\
  #   |             \      <<<Add>          |               |
  #   |             \      <Remove>>>       |               |
  #   |             \                       |               |
  #   |             \                       |               |
  #   ...
  #   \-------------/                       |               |
  #   Weight <-------->                     \---------------/
  class HangarEditor {
    inherit Mode
    variable nameBox
    variable addedList
    variable availList
    variable weightSlider
    variable catalogue
    variable onReturn

    constructor {cat ret} {
      Mode::constructor
    } {
      set catalogue $cat
      set onReturn $ret
      set nameBox [new ::gui::TextField [_ A editor ship_name] [$ str $cat.name]]
      set addedList [new ::gui::List [_ A hangar in_hangar] {} yes "$this selectionChange"]
      set availList [new ::gui::List [_ A hangar available] {} yes]
      set weightSlider none
      set weightSlider [new ::gui::Slider [_ A hangar weight] \
                        int {expr 1} {} 1 256 1 "$this sliderMoved"]

      # Produce list of ships; format: CLASS: NAME\a\aINTNAME\n
      set ships {}
      for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
        set shipName [$ str hangar.all_ships.\[$i\]]
        set mount ship:$shipName
        set cls [$ str $mount.info.class]
        set properName [$ str $mount.info.name]
        lappend ships [format "%s: %s\a\a%s\n" $cls $properName $shipName]
      }

      # For each item already in the contents, add it to the used list
      # and remove from the available list
      set usedShips {}
      for {set i 0} {$i < [$ length $catalogue.contents]} {incr i} {
        set sentinel "\a\a[$ str $catalogue.contents.\[$i\].target]\n"
        # Search for the sentinel
        for {set j 0} \
          {$j < [llength $ships] && -1 == [string first $sentinel [lindex $ships $j]]} \
          {incr j} {}
        # j is now at the correct index
        if {$j < [llength $ships]} {
          lappend usedShips [lindex $ships $j]
          set ships [lreplace $ships $j $j]
        } else {
          # Not a valid ship (??)
          $ remix $catalogue.contents $i
          incr i -1
        }
      }

      set mainPanel [new ::gui::BorderContainer 0 0.02]
      $mainPanel setElt top $nameBox
      set middlePane [new ::gui::VerticalContainer 0.03 centre]
      $middlePane add [new ::gui::Button [_ A hangar add] "$this addSelected"]
      $middlePane add [new ::gui::Button [_ A hangar rem] "$this removeSelected"]
      set leftPane [new ::gui::BorderContainer]
      $leftPane setElt centre $addedList
      $leftPane setElt bottom $weightSlider
      set rightPane [new ::gui::BorderContainer]
      $rightPane setElt centre $availList
      set okCancel [new ::gui::HorizontalContainer 0.02 right]
      set ok [new ::gui::Button [_ A gui ok] "$this okButton"]
      $ok setDefault
      $okCancel add $ok
      set can [new ::gui::Button [_ A gui cancel] "$this cancelButton"]
      $can setCancel
      $okCancel add $can
      $rightPane setElt bottom $okCancel
      $mainPanel setElt centre [new ::gui::DividedContainer $leftPane $middlePane $rightPane 0.03]
      set root [new ::gui::Frame $mainPanel]
      refreshAccelerators
      $root setSize 0 1 $::vheight 0

      $addedList setItems [lsort $usedShips]
      $availList setItems [lsort $ships]
    }


    # Strip the pretty text from a list entry and return the
    # internal ship name
    method deprettyShip text {
      set ix [string last "\a\a" $text]
      set len [string length $text]
      string range $text [expr {$ix+2}] [expr {$len-2}]
    }

    # Searches the catalogue for the given ship name;
    # returns the index of its entry or -1 if it does
    # not exist
    method findCatalogueEntry name {
      for {set i 0} {$i < [$ length $catalogue.contents]} {incr i} {
        if {$name == [$ str $catalogue.contents.\[$i\].target]} {
          return $i
        }
      }

      return -1
    }

    method addSelected {} {
      set sel [$availList getSelection]
      if {0 == [llength $sel]} return

      set items [$availList getItems]
      for {set i 0; set ci [$ length $catalogue.contents]} \
          {$i < [llength $sel]} {incr i; incr ci} {
        set ix [lindex $sel $i]
        set ship [deprettyShip [lindex $items $ix]]
        $ append $catalogue.contents STGroup
        $ adds $catalogue.contents.\[$ci\] target $ship
        $ addi $catalogue.contents.\[$ci\] weight 1
      }

      moveFromTo $availList $addedList
      $weightSlider setValue 1
    }

    method removeSelected {} {
      set sel [$addedList getSelection]
      if {0==[llength $sel]} return

      set items [$addedList getItems]
      for {set i 0} {$i < [llength $sel]} {incr i} {
        set ix [lindex $sel $i]
        set ship [deprettyShip [lindex $items $ix]]
        $ remix $catalogue.contents [findCatalogueEntry $ship]
      }

      moveFromTo $addedList $availList
    }

    # Transfer all selected items from fromList into
    # toList; fromList's selection will be empty, and
    # the moved items will be selected in toList
    method moveFromTo {from to} {
      set sel [$from getSelection]
      set toMove {}
      set froms [$from getItems]
      set tos [$to getItems]
      foreach ix $sel {
        lappend toMove [lindex $froms $ix]
        lappend tos [lindex $froms $ix]
      }
      foreach item $toMove {
        set ix [lsearch -exact $froms $item]
        set froms [lreplace $froms $ix $ix]
      }
      # Froms is still sorted, tos is not
      set tos [lsort $tos]
      # Generate new selection
      set sel {}
      foreach item $toMove {
        lappend sel [lsearch -exact $tos $item]
      }

      # Finish
      $to setItems $tos
      $from setItems $froms
      $to setSelection $sel
    }

    method selectionChange {} {
      set sel [$addedList getSelection]
      # If no selection, nothing to change
      if {0==[llength $sel]} return

      # Set the slider to the first item's weight
      set item [lindex [$addedList getItems] [lindex $sel 0]]
      set ship [deprettyShip $item]
      set ix [findCatalogueEntry $ship]
      $weightSlider setValue [$ int $catalogue.contents.\[$ix\].weight]
    }

    method sliderMoved args {
      if {$weightSlider == "none"} return

      # Change weights for all selected
      set sel [$addedList getSelection]
      set items [$addedList getItems]
      set val [$weightSlider getValue]
      foreach ix $sel {
        set item [lindex $items $ix]
        set ship [deprettyShip $item]
        set index [findCatalogueEntry $ship]
        $ seti $catalogue.contents.\[$index\].weight $val
      }
    }

    method okButton {} {
      $ sets $catalogue.name [$nameBox getText]
      $ sync hangar
      namespace eval :: $onReturn
      lappend ::gui::autodelete $this
    }
    method cancelButton {} {
      $ revert hangar
      namespace eval :: $onReturn
      lappend ::gui::autodelete $this
    }
  }
}
package provide gui $gui::version
