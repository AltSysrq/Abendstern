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
