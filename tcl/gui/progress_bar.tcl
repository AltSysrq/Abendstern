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
