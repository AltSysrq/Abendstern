#! /usr/bin/tclsh
# Simple script to automatically create a font package for Abendstern.
# First command-line argument is a filename pointing to a font definition,
# which is a Tcl script that takes a variable ch and sets font to the
# filename of the font to use for that character, or leave it unset
# to indicate the character is not to be emitted.
# Second is the output directory.

proc toUnicode ch {
  scan $ch %c ret
  return $ret
}

proc toChar uni {
  format %c $uni
}

set input [open [lindex $argv 0] r]
set script [read $input]
close $input

eval "proc accept {ch fn af} {
  upvar \$fn font
  upvar \$af auxflags
  $script
}"


for {set ch 1} {$ch < 65536} {incr ch} {
  set auxflags {}
  accept $ch font auxflags
  if {[info exists font]} {
    set chr [toChar $ch]
    if {$chr == "\\"} {set chr \\\\}
    exec convert -background transparent -fill white \
                 -font $font -pointsize 60 label:$chr \
                 -depth 1 \
                 {*}$auxflags \
                 [format "[lindex $argv 1]/%04x.png" $ch]
    unset font
  }
}
