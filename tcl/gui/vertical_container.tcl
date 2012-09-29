  # The VerticalContainer stacks its elements vertically.
  # The minimum width is the maximum minimum width of any element.
  # The minimum height is the sum of the minimum heights of the elements,
  # plus (n-1)*shim.
  # The width of each element is the width of the container.
  # The height of each element is the minimum height of the widget; this
  # means there may be left-over space at the bottom. The exception is when
  # the container is in grid mode, in which case all have the height
  # h/count.
  class VerticalContainer {
    inherit Container

    variable members
    variable shim
    variable mode

    constructor {{s 0} {mod top}} {Container::constructor} {
      set members {}
      set shim $s
      set mode $mod
    }

    method getMembers {} {return $members}
    method minWidth {} {
      set max 0
      foreach mem $members {
        set w [$mem minWidth]
        if {$w>$max} {set max $w}
      }
      return $max
    }

    method minHeight {} {
      if {$mode != "grid"} {
        set sum 0
        foreach mem $members {
          set sum [expr {$sum+[$mem minHeight]}]
        }
        return [expr {$sum+$shim*([llength $members]-1)}]
      } else {
        set max 0
        foreach mem $members {
          set h [$mem minHeight]
          if {$h > $max} { set max $h }
        }
        return [expr {($shim+$max)*[llength $members] - $shim}]
      }
    }

    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set getMinHeight {$mem minHeight}
      switch $mode {
        top {set y $t}
        bottom {set y [expr {$b+[minHeight]}]}
        centre -
        center {set y [expr {($t+$b)/2.0+[minHeight]/2.0}]}
        grid {set y $t; set getMinHeight "expr {($t-$b)/double([llength $members])}"}
      }
      foreach mem $members {
        set mh [eval $getMinHeight]
        set ny [expr {$y-$mh}]
        $mem setSize $l $r $y $ny
        set y [expr {$ny-$shim}]
      }
    }

    method add w {
      lappend members $w
    }
  }
