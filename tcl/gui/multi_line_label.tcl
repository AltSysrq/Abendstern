  # A MultiLineLabel acts much like a label, but wraps its text.
  # It is always left- and top-alligned.
  class MultiLineLabel {
    inherit AWidget

    variable text
    variable vminHeight

    constructor {{inittext {}} {lines 1}} {
      AWidget::constructor
    } {
      set text $inittext
      set vminHeight [expr {[$::gui::font getHeight]*$lines}]
    }

    method getText {} { return $text }
    method setText txt {
      set text $txt
    }

    method minHeight {} {
      return $vminHeight
    }

    method setMinHeight h {
      set vminHeight $h
    }

    method draw {} {
      set h [$::gui::font getHeight]
      set y [expr {$top - $h}]
      set width [expr {$right-$left}]
      set lines [split $text "\n"]
      ::gui::colourStd
      $::gui::font preDraw
      set currLine {}

      foreach line $lines {
        if {![string length [string trim $line]]} continue
        set words [split $line]
        set currLine {}

        foreach word $words {
          set word [string trim $word]
          if {![string length $word]} continue
          if {[$::gui::font width "$currLine $word"] > $width} {
            $::gui::font drawStr $currLine $left $y
            set y [expr {$y-$h}]
            set currLine $word
          } elseif {[string length $currLine]} {
            set currLine "$currLine $word"
          } else {
            set currLine $word
          }
        }

        if {[string length $currLine]} {
          $::gui::font drawStr $currLine $left $y
          set y [expr {$y-$h}]
          set currLine {}
        }
      }
      if {[string length $currLine]} {
        $::gui::font drawStr $currLine $left $y
        set y [expr {$y-$h}]
      }
      $::gui::font postDraw
    }
  }
