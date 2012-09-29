  # A TextField allows the user to supply an arbitrary
  # single line of text. An ActivatorLabel is used at
  # left, and TextFieldImpl in the centre. Using the
  # mouse to focus the text field and locate the cursor
  # is supported; key bindings are as follows:
  #   left  Move cursor one character left
  #   right Move cursor one character right
  #   C-lft One word left
  #   C-rht One word right
  #   home  Move cursor to position zero
  #   end   Move cursor past last character
  #   back  Delete character to left of cursor
  #   C-bk  Delete word to left of cursor
  #   M-bk  Delete word to left of cursor
  #   del   Delete character to right of cursor
  #   C-del Delete word to right of cursor
  #   M-del Delete word to right of cursor
  #   tab   Defocus
  #   esc   Defocus
  #   enter Action and defocus
  #   C-f   One char right
  #   C-b   One char left
  #   M-f   One word right (skip non-alnum, then alnum)
  #   M-b   One word left
  #   C-a   Position zero
  #   C-e   Past last character
  #   C-d   Delete character to right of cursor
  #   M-d   Delete word to right of cursor
  #   C-k   Delete all characters to right of cursor
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
        DOWN:?-C?:k_b -
        DOWN:?--?:k_left {
          if {$cursor > 0} {moveCursor [expr {$cursor-1}]}
        }
        DOWN:?-C?:k_f -
        DOWN:?--?:k_right {
          if {$cursor < [string length $contents]} {
            moveCursor [expr {$cursor+1}]
          }
        }
        DOWN:?A-?:k_b -
        DOWN:?A-?:k_left -
        DOWN:?-C?:k_left {
          set ix $cursor
          set lastWordChar $cursor
          if {$ix > 0} {
            incr ix -1
          }
          while {$ix > 0 && ![string is alnum [string index $contents $ix]]} {
            incr ix -1
            # Always move over the non-words
            set lastWordChar $ix
          }
          while {$ix > 0 && [string is alnum [string index $contents $ix]]} {
            # Save the current index as a word character in case we pass the
            # word
            set lastWordChar $ix
            incr ix -1
          }

          # We want to land at the beginning of the word, so move forward to
          # $lastWordChar if the current character is not a word char
          if {![string is alnum [string index $contents $ix]]} {
            set ix $lastWordChar
          }

          moveCursor $ix
        }
        DOWN:?A-?:k_f -
        DOWN:?A-?:k_right -
        DOWN:?-C?:k_right {
          set ix $cursor
          set l [string length $contents]
          while {$ix < $l && ![string is alnum [string index $contents $ix]]} {
            incr ix +1
          }
          while {$ix < $l && [string is alnum [string index $contents $ix]]} {
            incr ix +1
          }

          moveCursor $ix
        }
        DOWN:?-C?:k_a -
        DOWN:????:k_home { moveCursor 0 }
        DOWN:?-C?:k_e -
        DOWN:????:k_end  { moveCursor [string length $contents] }
        DOWN:?--?:k_backspace -
        DOWN:?-C?:k_h {
          if {$cursor > 0} {
            set prefix [string range $contents 0 [expr {$cursor-2}]]
            set postfix [string range $contents $cursor [string length $contents]]
            set contents $prefix$postfix
            moveCursor [expr {$cursor-1}]
          }
        }
        DOWN:?-C?:k_backspace -
        DOWN:?A-?:k_backspace -
        DOWN:?A-?:k_h {
          set end $cursor
          set ix $cursor
          set lastWordChar $ix
          if {$ix > 0} {
            incr ix -1
          }
          while {$ix > 0 && ![string is alnum [string index $contents $ix]]} {
            incr ix -1
            # Always move over the non-words
            set lastWordChar $ix
          }
          while {$ix > 0 && [string is alnum [string index $contents $ix]]} {
            # Save the current index as a word character in case we pass the
            # word
            set lastWordChar $ix
            incr ix -1
          }

          # We want to land at the beginning of the word, so move forward to
          # $lastWordChar if the current character is not a word char
          if {![string is alnum [string index $contents $ix]]} {
            set ix $lastWordChar
          }

          set prefix [string range $contents 0 $ix-1]
          set suffix [string range $contents $end end]
          set contents $prefix$suffix
          moveCursor [expr {$ix}]
        }
        DOWN:?-C?:k_d -
        DOWN:?--?:k_delete {
          if {$cursor < [string length $contents]} {
            set prefix [string range $contents 0 [expr {$cursor-1}]]
            set postfix [string range $contents [expr {$cursor+1}] [string length $contents]]
            set contents $prefix$postfix
          }
        }
        DOWN:?A-?:k_d -
        DOWN:?A-?:k_delete -
        DOWN:?-C?:k_delete {
          set end $cursor
          set ix $cursor

          set l [string length $contents]
          while {$ix < $l && ![string is alnum [string index $contents $ix]]} {
            incr ix +1
          }
          while {$ix < $l && [string is alnum [string index $contents $ix]]} {
            incr ix +1
          }

          set prefix [string range $contents 0 $end-1]
          set suffix [string range $contents $ix end]
          set contents $prefix$suffix
        }
        DOWN:?-C?:k_k {
          set contents [string range $contents 0 $cursor-1]
        }
        DOWN:????:k_tab -
        DOWN:????:k_escape {loseFocus}
      }
    }

    method character ch {
      if {$justFocused} return
      if {![isFocused]} return
      if {[toUnicode $ch] >= 0x20 && [toUnicode $ch] != 0x7F &&
          [string match ---? $::currentKBMods]} {
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
