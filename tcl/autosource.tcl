# This file is automatically safe_sourced at the end of bridge.tcl

encoding system utf-8

# Eval this so that globalConf gets subbed correctly
eval "proc \$ {args} { $globalConf {*}\$args }"
safe_source tcl/gui.tcl

proc toUnicode ch {
  scan $ch %c ret
  return $ret
}

proc toChar uni {
  format %c $uni
}

proc glVertex2f {x y} {
  glVertex $x $y
}

proc glColor3f {r g b} {
  glColour $r $g $b 1
}

proc glColor4f {r g b a} {
  glColour $r $g $b $a
}

proc glColour3f {r g b} {
  glColour {$r $g $b 1}
}

proc glColour4f {r g b a} {
  glColour {$r $g $b $a}
}

proc glTranslatef {x y z} {
  glTranslate $x $y
}

proc glRotatef {th x y z} {
  glRotate $th
}

proc glScalef {x y z} {
  glScale $x $y
}

# Convert "foo.ship" or "hangar/x/foo.ship" to "ship:x/foo"
proc shipPath2Mount file {
  set file [unhomeq $file]
  if {-1 != [set ix [string first / $file]]} {
    set file [string range $file $ix+1 end]
  }
  return ship:[string range $file 0 end-5]
}

# Convert "ship:x/foo" to "hangar/x/foo.ship"
proc shipMount2Path mount {
  homeq hangar/[string range $mount 5 end].ship
}

# Convert "ship:x/foo" to "x/foo"
proc shipMount2Name mount {
  string range $mount 5 end
}

# Convert "hangar/x/foo.ship" to "x/foo"
proc shipPath2Name path {
  shipMount2Name [shipPath2Mount $path]
}

# Convert "x/foo" to "ship:x/foo"
proc shipName2Mount name {
  return ship:$name
}

# Convert "x/foo" to "hangar/x/foo.ship"
proc shipName2Path name {
  homeq hangar/$name.ship
}

# Set the specified variable to the given new value,
# then delete the old.
proc exch {var val} {
  upvar $var v
  if {[catch {set old $v}]} { set old {}}
  set v $val
  if {[string length $old] && $old != "0"} {
    delete object $old
  }
}

# Same as exch, but works with public variables
proc oexch {obj var val} {
  if {[catch {set old [$obj cget -$var]}]} {set old {}}
  $obj configure -$var $val
  if {[string length $old] && $old != "0"} {
    delete object $old
  }
}

# Returns the qualified path of the given path relative to the user's
# abendstern home directory.
# eg, [homeq foo/bar] ==> /home/jlinge/.abendstern/foo/bar
proc homeq {rel} {
  file join $::env(HOME) .abendstern $rel
}

# Removes the prefix added by homeq, if any is present.
proc unhomeq {path} {
  set pfx [string range [homeq a] 0 end-1]
  if {0 == [string first $pfx $path]} {
    set path [string range $path [string length $pfx] end]
  }
  return $path
}

# Loop construct for the common ranged loops.
# Usage:
#   do variable from-inclusive to-exclusive body
# from-inclusive and to-inclusive are used literally in the for condition.
#
# Examples:
#   do i 0 5 { puts $i }
#   0
#   1
#   2
#   3
#   4
#
#   # Config: root [0 1 2 3]
#   do i [$ length root] { puts [$ int root.\[$i\]]; $ remove root.\[$i\] }
#   0
#   2
#   error
#
#   do i {[$ length root]} { # same... }
#   0
#   2
#   (end of loop)
proc do {var from to body} {
  uplevel 1 [list for "set {$var} $from" "\${$var} < $to" "incr {$var}" $body]
}

# Iterates over items in a libconfig collection.
# The var is set to entries in the config; eg, foo.[0], foo.[1], ...
# It cannot handle concurrent modification of the list, though.
proc conffor {var conf body} {
  upvar $var v
  do i 0 [$ length $conf] {
    set v $conf.\[$i\]
    uplevel $body
  }
}
