  # A Label simply displays fixed text. It does not take an accelerator.
  # It can be left-, centre-, or right-oriented.
  # It is always centred vertically.
  class Label {
    inherit AWidget
    variable orient

    constructor {text {orientation centre}} {AWidget::constructor} {
      setText $text
      setOrientation $orientation
    }

    method getText {} {return $name}
    method setText t {
      set name $t
      set displayName $t
    }
    method getOrientation {} {
      return $orient
    }
    method setOrientation or {
      switch -- $or {
        left - right - centre {set orient $or}
        center {set orient centre}
        default {error "Unknown orientation: $or"}
      }
    }

    method minHeight {} {
      $::gui::font getHeight
    }
    method minWidth {} {
      $::gui::font width $displayName
    }

    method setAccelerator p { return $p }

    method draw {} {
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set y [expr {$bottom+$h/2.0-[minHeight]/2.0}]
      switch $orient {
        left {set x $left}
        right {set x [expr {$right-[minWidth]}]}
        centre {set x [expr {$left+$w/2.0-[minWidth]/2.0}]}
      }
      set f $::gui::font
      ::gui::colourStd
      $f preDraw
      $f drawStr $displayName $x $y
      $f postDraw
    }

    method drawCompiled {} {
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set y [expr {$bottom+$h/2.0-[minHeight]/2.0}]
      switch $orient {
        left {set x $left}
        right {set x [expr {$right-[minWidth]}]}
        centre {set x [expr {$left+$w/2.0-[minWidth]/2.0}]}
      }
      ::gui::ccolourStd
      cglText $displayName $x $y
    }
  }
