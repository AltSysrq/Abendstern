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
