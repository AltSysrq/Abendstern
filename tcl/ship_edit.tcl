package require Itcl
package require gui

namespace import ::itcl::*

# This file defines the ShipEditor Application, which is a thin
# container for a Manipulator and the several Modes the editor
# uses.
class ShipEditor {
  inherit ::gui::Application

  variable retval
  variable manipulator

  constructor {} {Application::constructor} {
    $ open tcl/editor/scratch.dat edit
    set retval 0
    set manipulator [new Manipulator default]
    set mode [new ShipEditorInitMode $this]
  }

  destructor {
    delete object $manipulator
    $ revert edit
    $ close edit
  }

  method updateThis et {
    $manipulator update
    return $retval
  }

  method drawThis {} {
    $manipulator draw
  }

  method setSubState app {
    set subapp $app
  }

  method setMode md {
    lappend ::gui::autodelete $mode
    set mode $md
  }

  method die {} {
    set retval $this
  }

  method manip args {
    $manipulator {*}$args
  }
}

# Generates a new basename from the given ship name.
# The name will be less than or equal to 8 letters
# long and be strictly ASCII alphanumeric lowercase, and
# will not begin with a numeral.
# A userid (or blank for local account) can be specified
# to generate a name for a user other than the current.
proc generateNewShipBasename {name {userid x}} {
  if {"x" == $userid} { set userid $::abnet::userid }
  # Begin by making it lowercase
  set name [string tolower $name]
  # Replace common non-ASCII letters with
  # Romanised equivalents
  # This only has lowercase
  foreach {letter replacement} {
    ä ae   ö oe   ü ue   ß ss
    á a    é e    í i    ó o    ú u    ñ n
    α a    β b    γ g    δ d    ε e    ζ z
    η i    θ th   ι i    λ l    μ m    ν n
    ξ x    ο o    π p    ρ r    σ s    τ t
    υ u    φ ph   χ ch   ψ ps   ω ou
    а a    б b    ц c    д d    е e
    ф f    г g    х x    и i    й j
    к k    л l    м m    н n    о o
    п p    я ja   р r    с s    т t
    у u    ж zh   в w    ь {}   ы i
    з z    ш sh   щ sh   э e    ё jo
    ч ch   ю ju
  } {
    set name [regsub -all -- $letter $name $replacement]
  }
  # Make ASCII alphanumeric
  for {set i 0} {$i < [string length $name]} {incr i} {
    set ch [string index $name $i]
    if {![string is alnum $ch] || ![string is ascii $ch]} {
      # Delete this character
      set name [string range $name 0 $i-1][string range $name $i+1 end]
      incr i -1
    }
  }

  # If zero-length, prepend 'Q'
  if {0 == [string length $name]} {
    set name q$name
  }

  # If it starts with a numeral, prepend 'X'
  if {[string is digit [string index $name 0]]} {
    set name x$name
  }

  # Is it unique?
  # If not, begin appending numbers until we find one that is
  set unique no
  set num ""
  if {"" == $userid} {
    set prefix local/
  } else {
    set prefix $userid/
  }
  while {!$unique} {
    set unique yes
    set wname $prefix[string range $name 0 [expr {7-[string length $num]}]]$num
    for {set i 0} {$i < [$ length hangar.all_ships] && $unique} {incr i} {
      if {$wname == [$ str hangar.all_ships.\[$i\]]} {
        set unique no
      }
    }

    # If we found another ship with the same name, we are not unique and
    # must try another name.
    if {!$unique} {
      if {[string length $num]} {
        incr num
      } else {
        set num 0
      }
    }
  }
  return $wname
}

source tcl/editor/initmode.tcl
source tcl/editor/mainmode.tcl
source tcl/editor/newmode.tcl
source tcl/editor/delmode.tcl
