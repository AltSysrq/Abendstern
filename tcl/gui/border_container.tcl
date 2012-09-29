  # A BorderContainer holds up to 5 widgets, in the following
  # layout:
  #   [___Top___]
  #   |L|     |R|
  #   |e| Ce  |i|
  #   |f| nt  |g|
  #   |t| re  |h|
  #   | |_____|t|
  #   [_Bottom__]
  # In words:
  #   The top and bottom's heights are exactly their minimum heights.
  #   The left and right's widths are exactly their minimum widths.
  #   The minimum height is h(top)+h(bottom)+max(h(left),h(right),h(centre))
  #   The minimum width is w(left)+w(right)+max(w(top),w(bottom),w(centre))
  #   The width of the top and bottom is the width of the entire widget.
  #   The width of the centre is width-w(left)-w(right)
  #   The height of the left, right, and centre is height-h(top)-h(bottom).
  # The order of the widgets is top, left, centre, right, bottom, unless
  # the makeCentreLast method is called, in which case centre is moved to
  # the end.
  class BorderContainer {
    inherit Container

    variable top
    variable left
    variable centre
    variable right
    variable bottom
    variable xshim
    variable yshim
    variable centreLast

    constructor {{xs 0} {ys 0}} {Container::constructor} {
      foreach v {top left centre right bottom} {
        set $v [new ::gui::AWidget]
      }
      set xshim $xs
      set yshim $ys
      set centreLast no
    }

    method getMembers {} {
      if {$centreLast} {
        list $top $left $right $bottom $centre
      } else {
        list $top $left $centre $right $bottom
      }
    }

    method makeCentreLast {} { set centreLast yes }

    method minWidth {} {
      expr {[$left minWidth]+[$right minWidth]+
            max([$top minWidth],[$centre minWidth],[$bottom minWidth])+2*$xshim}
    }
    method minHeight {} {
      expr {[$top minHeight]+[$bottom minHeight]+
            max([$left minHeight],[$centre minHeight],[$bottom minHeight])+2*$yshim}
    }
    method setSize {l r t b} {
      AWidget::setSize $l $r $t $b
      set w [expr {$r-$l}]
      set h [expr {$t-$b}]
      set tl [expr {$t-[$top minHeight]}]
      set bh [expr {$b+[$bottom minHeight]}]
      set lr [expr {$l+[$left minWidth]}]
      set rl [expr {$r-[$right minWidth]}]
      $top setSize $l $r $t $tl
      $left setSize $l $lr [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $centre setSize [expr {$lr+$xshim}] [expr {$rl-$xshim}] [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $right setSize $rl $r [expr {$tl-$yshim}] [expr {$bh+$yshim}]
      $bottom setSize $l $r $bh $b
    }

    # Set the given element. The old value will be deleted first.
    # We also honour Java-style compass elements.
    method setElt {elt val} {
      switch -- $elt {
        top - north {set e top}
        left - west {set e left}
        centre - center {set e centre}
        right - east {set e right}
        bottom - south {set e bottom}
        default { error "Unrecognized element: $elt" }
      }
      delete object [set $e]
      set $e $val
    }
  }
