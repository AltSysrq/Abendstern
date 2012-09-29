  # A SpiderGraph (I don't know what the correct name is) plots values radially,
  # relative to a given "normal" value for each radius. Anything exactly equal to
  # the normal for a radius has half the length of that radius. The radius of any
  # other value is given by (where R=total radius, v=value, n=normal)
  #   R*(1-0.5*n/v)   When v>n
  #   R*(v/n/2)       When v<n
  # To the right of the graph is a label. Without interaction, it highlights and
  # shows the name of a particular radius, cycling once every two seconds. When
  # the mouse is over the graph, it displays the name of the radius closest to
  # the mouse cursor.
  # The SpiderDatum class holds the data for each radius of a SpiderGraph.
  class SpiderDatum {
    variable name
    variable normal
    public variable value

    constructor {nam norm {val 0}} {
      set name $nam
      set normal $norm
      set value $val
    }

    method getName {} {return $name}
    method getNorm {} {return $normal}
    method getAdjustedValue {} {
      if {[catch {
        set v [expr {$value > $normal? 1.0-0.5*$normal/$value : $value/$normal/2.0}]
      }]} {
        error "expr {$value > $normal? 1.0-0.5*$normal/$value : $value/$normal/2.0}"
      }
      return $v
    }

    method fqn {} {return $this}
  }
  class SpiderGraph {
    inherit AWidget

    variable data
    variable mouseOver
    variable currentDisplay
    variable timeUntilDisplaySwitch
    variable radius

    constructor dat {
      AWidget::constructor
    } {
      set data $dat
      set mouseOver no
      set currentDisplay 0
      set timeUntilDisplaySwitch 2000
    }

    destructor {
      foreach datum $data { delete object $datum }
    }

    method minHeight {} {
      expr {[$::gui::font getHeight]*2.5}
    }
    method minWidth {} {
      set maxLabelWidth 0
      foreach datum $data {
        set w [$::gui::font width "M[$datum getName]"]
        if {$w > $maxLabelWidth} {
          set maxLabelWidth $w
        }
      }

      expr {[minHeight]+$maxLabelWidth}
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set radius [expr {($t-$b)/2.0}]
    }

    method motion {x y buttons} {
      set cy [expr {($bottom+$top)/2.0}]
      if {$x >= $left && $x <= ($left+2*$radius)
      &&  $y >= $cy-$radius && $y <= $cy+$radius} {
        set mouseOver yes

        set cx [expr {$left+$radius}]
        set rx [expr {$x-$cx}]
        set ry [expr {$y-$cy}]
        set angle [expr {atan2($ry,$rx)}]
        set pi [expr {acos(-1)}]
        # Modify angle from -pi..pi to 0..2*pi
        if {$angle < 0} {set angle [expr {$angle+2*$pi}]}
        set anglePerDatum [expr {2*$pi/[llength $data]}]
        # By rounding angle/anglePerDatum, we get the nearest
        # datum, EXCEPT in the case of the upper half of the
        # segment approaching 2*pi, in which case we actually
        # use datum 0
        set ix [expr {round($angle/$anglePerDatum)}]
        if {$ix == [llength $data]} {set ix 0}

        # Yay
        set currentDisplay $ix
        set timeUntilDisplaySwitch 0
      } else {
        set mouseOver no
      }
    }

    method draw {} {
      # Update first
      global applicationUpdateTime
      if {!$mouseOver} {
        set timeUntilDisplaySwitch [expr {$timeUntilDisplaySwitch-$applicationUpdateTime}]
        if {$timeUntilDisplaySwitch < 0} {
          set timeUntilDisplaySwitch 2000
          set currentDisplay [expr {($currentDisplay+1)%[llength $data]}]
        }
      }

      set cx [expr {$left+$radius}]
      set cy [expr {($top+$bottom)/2.0}]
      set pi [expr {acos(-1)}]
      set anglePerDatum [expr {2*$pi/[llength $data]}]

      # Draw the graph itself
      # Since it will be concave much of the time, we must use
      # a triangle fan instead of a simple polygon
      ::gui::colourSpec
      glBegin GL_TRIANGLE_FAN
      glVertex2f $cx $cy
      # Loop through the data; we must draw the first datum as the
      # last to complete the figure
      for {set i 0} {$i <= [llength $data]} {incr i} {
        set datum [lindex $data [expr {$i%[llength $data]}]]
        set amt [$datum getAdjustedValue]
        set angle [expr {$anglePerDatum*$i}]
        # Colour towards spec as we approach zero, and toward
        # dang as we approach one, with std at the middle.
        if {$amt < 0.5} {
          ::gui::blendedColours special [expr {1.0-$amt*2}] standard [expr {$amt*2}]
        } else {
          ::gui::blendedColours danger [expr {($amt-0.5)*2}] standard [expr {1.0-($amt-0.5)*2}]
        }
        glVertex2f [expr {$cx+$radius*$amt*cos($angle)}] \
                   [expr {$cy+$radius*$amt*sin($angle)}]
      }
      glEnd

      # Highlight current datum
      ::gui::colourWhit
      glBegin GL_LINES
      glVertex2f $cx $cy
      set angle [expr {$anglePerDatum*$currentDisplay}]
      glVertex2f [expr {$cx+$radius*cos($angle)}] [expr {$cy+$radius*sin($angle)}]
      glEnd

      # Draw text of current item
      ::gui::colourStd
      $::gui::font preDraw
      $::gui::font drawStr [[lindex $data $currentDisplay] getName] \
                           [expr {$cx+$radius+[$::gui::font charWidth M ]}] \
                           [expr {$cy-[$gui::font getHeight]/2.0}]
      $::gui::font postDraw
    }
  }
