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
