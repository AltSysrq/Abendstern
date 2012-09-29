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

