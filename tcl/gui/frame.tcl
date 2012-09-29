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
