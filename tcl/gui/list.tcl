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
      BorderContainer::constructor 0 0.001
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

    # Scrolls the list so that the specified index is visible
    method scrollTo ix {
      $centre setIndex $ix
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
        if {$i >= 0 && $i < [llength $selected]} {
          set selected [lreplace $selected $i $i yes]
        }
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
