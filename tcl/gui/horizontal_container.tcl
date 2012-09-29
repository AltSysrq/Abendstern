  # A HorizontalContainer is in every way identical to
  # a VerticalContainer, except operating on the X axis
  class HorizontalContainer {
    inherit Container
    variable members
    variable shim
    variable mode
    constructor {{s 0} {mod left}} {Container::constructor} {
      set memberl {}
      set shim $s
      set mode $mod
    }

    method getMembers {} {return $members}
    method minWidth {} {
      if {$mode != "grid"} {
        set sum 0
        foreach mem $members {
          set sum [expr {$sum+[$mem minWidth]}]
        }
        return [expr {$sum+$shim*([llength $members]-1)}]
      } else {
        set max 0
        foreach mem $members {
          set w [$mem minWidth]
          if {$w > $max} { set max $w }
        }
        return [expr {($shim+$max)*[llength $members] - $shim}]
      }
    }

    method minHeight {} {
      set max 0
      foreach mem $members {
        set h [$mem minHeight]
        if {$h>$max} {set max $h}
      }
      return $max
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set getMinWidth {$mem minWidth}
      switch $mode {
        left {set x $l}
        right {set x [expr {$r-[minWidth]}]}
        centre -
        center {set x [expr {($l+$r)/2.0-[minWidth]/2.0}]}
        grid {set x $l; set getMinWidth "expr {($r-$l)/double([llength $members])-$shim}"}
      }
      foreach mem $members {
        set nx [expr {$x+[eval $getMinWidth]}]
        $mem setSize $x $nx $t $b
        set x [expr {$nx+$shim}]
      }
    }

    method add w {
      lappend members $w
    }
  }
