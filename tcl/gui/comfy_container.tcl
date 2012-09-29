  # A ComfyContainer houses a single AWidget, whose size
  # is shrunken by a given factor. Both factors may be
  # controlled, but the Y factor defaults to X's
  class ComfyContainer {
    inherit Container
    variable w
    variable xfactor
    variable yfactor
    constructor {child {xfac 0.8} {yfac -1}} {
      Container::constructor
    } {
      set w $child
      set xfactor $xfac
      if {$yfac < 0} {
        set yfactor $xfac
      } else {
        set yfactor $yfac
      }
    }

    method getMembers {} {return $w}
    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set cx [expr {($l+$r)/2.0}]
      set cy [expr {($t+$b)/2.0}]
      set rx [expr {($r-$l)/2.0}]
      set ry [expr {($t-$b)/2.0}]
      $w setSize [expr {$cx-$rx*$xfactor}] [expr {$cx+$rx*$xfactor}] \
                 [expr {$cy+$ry*$yfactor}] [expr {$cy-$ry*$yfactor}]
    }
    method minWidth {} {$w minWidth}
    method minHeight {} {$w minHeight}

    # Resets the factors to match the minimum size of the inner
    # component.
    method pack {} {
      packx
      packy
    }

    # Packs only the X dimension
    method packx {} {
      set xfactor [expr {[minWidth]/($right-$left)}]
      setSize $left $right $top $bottom
    }

    # Packs only the Y dimension
    method packy {} {
      set yfactor [expr {[minHeight]/($top-$bottom)}]
      setSize $left $right $top $bottom
    }
  }
