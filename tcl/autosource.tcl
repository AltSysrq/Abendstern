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
  if {-1 != [set ix [string first / $file]]} {
    set file [string range $file $ix+1 end]
  }
  return ship:[string range $file 0 end-5]
}

# Convert "ship:x/foo" to "hangar/x/foo.ship"
proc shipMount2Path mount {
  return hangar/[string range $mount 5 end].ship
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
  return hangar/$name.ship
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
