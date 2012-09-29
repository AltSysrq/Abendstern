  # The DividedContainer contains exactly three widgets: left, centre,
  # and right. The centre widget is exactly its minimum width and is
  # perfectly centred within the container, and the left and right
  # widgets fill the rest of the space on the sides. All widgets have
  # the same height as the container. The minimum width is the minimum
  # width of the centre plus the maximum of the minima of the sides.
  # The minimum height is the greatest minimum height of any widget.
  class DividedContainer {
    inherit Container
    variable left
    variable centre
    variable right
    variable shim

    constructor {l c r {xs 0}} {
      Container::constructor
    } {
      set left $l
      set centre $c
      set right $r
      set shim $xs
    }

    method getMembers {} {
      list $left $centre $right
    }

    method minWidth {} {
      expr {2*$shim + [$centre minWidth] + 2*max([$left minWidth], [$right minWidth])}
    }

    method minHeight {} {
      expr {max(max([$left minHeight], [$centre minHeight]), [$right minHeight])}
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set centreW [$centre minWidth]
      $centre setSize [expr {($l+$r)/2.0-$centreW/2.0}] \
                      [expr {($l+$r)/2.0+$centreW/2.0}] \
                      $t $b
      $left  setSize $l [expr {($l+$r)/2.0-$centreW/2.0-$shim}] $t $b
      $right setSize [expr {($l+$r)/2.0+$centreW/2.0+$shim}] $r $t $b
    }
  }
