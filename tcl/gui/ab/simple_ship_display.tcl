  # Displays a Ship, scaled to fit within the boundaries.
  # If the ship is "error", a danger X is shown instead
  class SimpleShipDisplay {
    inherit AWidget

    variable ship

    constructor {} {AWidget::constructor} {set ship ""}

    method setShip sh {
      set ship $sh
      if {$ship != "" && $ship != "error"} {$ship teleport 0 0 0}
    }

    method draw {} {
      global ::cameraZoom
      set cameraZoom 1
      set w [expr {$right-$left}]
      set h [expr {$top-$bottom}]
      set minDim [expr {min($w,$h)}]
      set cx [expr {($right+$left)/2.0}]
      set cy [expr {($top+$bottom)/2.0}]

      if {$ship == ""} {return} \
      elseif {$ship == "error"} {
        ::gui::colourDang
        glBegin GL_LINES
          glVertex2f $left $top
          glVertex2f $right $bottom
          glVertex2f $right $top
          glVertex2f $left $bottom
        glEnd
        return
      }

      # Normal ship
      set shipDim [expr {[$ship getRadius]*2.0}]
      # Scale out, unless ship already fits
      set scale [expr {min(1.0, $minDim/$shipDim)}]

      # Draw
      glPushMatrix
      glTranslatef $cx $cy 0
      glScalef $scale $scale 1
      $ship draw
      glPopMatrix
    }
  }
