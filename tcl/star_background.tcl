# Contains procs for drawing a 3D "flying through space"
# effect for a background;
#
# I do not fully understand how all the math in this
# file works, so it is not documented (it is based
# on something I concocted when still beginning
# programming, but works surprisingly well).

package require Itcl

namespace eval sbg {
  set SPEED 0.00005
  set COUNT 75
  set DIST_FROM_CENTRE_OFF [expr {100.0/1280.0}]
  set DIST_FROM_CENTRE_OFF2 [expr {50/1280.0}]

  class StarDatum {
    variable x
    variable y
    variable vx
    variable vy
    variable distFromCentre

    constructor {distFromCentreOff cos sin basedist} {
      set cx 0.5
      set cy [expr {$::vheight/2}]

      set distFromCentre [expr {$basedist + $::sbg::DIST_FROM_CENTRE_OFF}]
      set velocity [expr {$::sbg::SPEED*0.5/$distFromCentre}]
      set distFromCentre [expr {$distFromCentre - $::sbg::DIST_FROM_CENTRE_OFF + $distFromCentreOff}]

      set vx [expr {$velocity*$cos}]
      set vy [expr {$velocity*$sin}]
      set x [expr {$cx + $distFromCentre*$cos}]
      set y [expr {$cy + $distFromCentre*$sin}]
    }

    method getVelocities {{ovxname ovx} {ovyname ovy}} {
      upvar $ovxname ovx
      upvar $ovyname ovy
      set ovx $vx
      set ovy $vy
      set dx [expr {0.5-$x}]
      set dy [expr {$::vheight/2.0-$y}]
      set dist [expr {sqrt($dx*$dx + $dy*$dy)}]
      set distFromCentre $dist
      set ovx [expr {$ovx*$dist*2.0}]
      set ovy [expr {$ovy*$dist*2.0}]
    }

    method update et {
      getVelocities ovx ovy
      set x [expr {$x+$ovx*$et}]
      set y [expr {$y+$ovy*$et}]
    }

    # Assumes that GL_LINES has began and the colour is set appropriately
    method draw {} {
      glVertex $x $y
    }

    method isAlive {} {
      expr {$x > 0 && $x < 1 && $y > 0 && $y < $::vheight}
    }

    method fqn {} {return $this}
  }

  class Star {
    variable lead
    variable tail
    variable time

    constructor {} {
      set angle [expr {rand()*2.0*3.1415926}]
      set cos [expr {cos($angle)}]
      set sin [expr {sin($angle)}]
      set basedist [expr {rand()/2.0}]
      set time 0

      set lead [new ::sbg::StarDatum $::sbg::DIST_FROM_CENTRE_OFF $cos $sin $basedist]
      set tail [new ::sbg::StarDatum $::sbg::DIST_FROM_CENTRE_OFF2 $cos $sin $basedist]
    }

    destructor {
      delete object $lead
      delete object $tail
    }

    method update et {
      set time [expr {$time+$et}]
      $lead update $et
      $tail update $et
    }

    method isAlive {} {
      $tail isAlive
    }

    # Assumes GL_LINES already began
    method draw {} {
      set brightness [expr {min(1.0, $time/1000.0)}]
      glColour $brightness $brightness $brightness 1
      $lead draw
      glColour 0 0 0 1
      $tail draw
    }

    method fqn {} {return $this}
  }

  # Init
  set stars {}
  for {set i 0} {$i < $COUNT} {incr i} {
    lappend stars [new ::sbg::Star]
  }

  proc update et {
    for {set i 0} {$i < [llength $::sbg::stars]} {incr i} {
      [lindex $::sbg::stars $i] update $et
      if {![[lindex $::sbg::stars $i] isAlive]} {
        delete object [lindex $::sbg::stars $i]
        lset ::sbg::stars $i [new ::sbg::Star]
      }
    }
  }

  proc draw {} {
    glBegin GL_LINES
    foreach star $::sbg::stars {
      $star draw
    }
    glEnd
  }

  # Finish init
  # Skip 1 second so the weird starting effect does not happen
  for {set i 0} {$i < 2} {incr i} {
    update 500
  }
}
