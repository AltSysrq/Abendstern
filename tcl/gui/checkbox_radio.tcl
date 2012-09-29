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

    # Directly alters the checked state, without triggering any kind of events.
    method forceChecked {c} {
      set checked $c
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
