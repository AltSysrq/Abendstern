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
