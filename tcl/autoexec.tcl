package require Itcl
namespace import ::itcl::*
package require gui

set THREAD foreground

# Pre-BootManager operations

# Write any output needed with the log function
if {$PLATFORM == "WINDOWS"} {
  # On Windows, we don't get stdout or stderr...
  # (Append since we write to the log already opened
  #  by C++)
  set logOutput [open log.txt a]
  proc log msg {
    puts $::logOutput $msg
    flush $::logOutput
  }
  proc closeLog {} { close $::logOutput }
} else {
  proc log msg { puts $msg }
  proc closeLog {} {}
}

# Load language information
# Always have en as fallback
l10n_acceptLanguage [$ str conf.language]
l10n_acceptLanguage en
foreach cat [list A N T] {
  foreach f [glob -nocomplain data/lang/*/$cat] {
    l10n_loadCatalogue $cat $f
  }
}

# Need to initialise graphics in non-headless
if {!$headless} glReset

# BootManager needs background
source tcl/star_background.tcl

# Secondary functions

# Force all owned ships to be uploaded next time
proc forceUploadAll {} {
  for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
    if {0 == [string first $::abnet::userid/ [$ str hangar.all_ships.\[$i\]]]} {
      $ setb [shipName2Mount [$ str hangar.all_ships.\[$i\]]].info.needs_uploading yes
    }
  }
}

# Returns a temporary filename not yet in use
proc tmpname {{pfx temp}} {
  set name $pfx[expr {int(rand()*65536*256)}]
  while {[file exists $name]} {
    set name $pfx[expr {int(rand()*65536*256)}]
  }
  return $name
}

# Tcl 8.5 does not have {file tempfile}.
# This is mostly the same, except that
# THE FILE WILL NOT BE DELETED AUTOMATICALLY ON CLOSING.
# As such, the variable is NOT optional.
proc mktemp {var {pfx temp}} {
  upvar $var name
  set name [tmpname $pfx]
  return [open $name wb+]
}

# Test the GameAdvertiser and GameDiscoverer classes.
# This proc does not return; it loops until the program
# is killed.
proc testGameDiscovery {} {
  set asm [new NetworkAssembly default 0 $::antenna]
  set oid [expr {int(rand()*1000)}]
  $asm addPacketProcessor \
    [new GameAdvertiser default [$asm getTuner] no $oid 10 no TEST]
  $asm addPacketProcessor \
    [set disc [new GameDiscoverer default [$asm getTuner]]]
  $disc start
  while {true} {
    $asm update 5
    $disc poll $::antenna
    if {1.0 == [$disc progress]} {
      $disc dumpResults
      $disc start
    }
    after 5
  }
}

proc makePlanet {} {
  set parms [new PlanetGeneratorParms default]
  #$parms configure -continents 16 -oceans 32 -largeIslands 4 -seas 2 -smallIslands 32 -lakes 32 \
  #                 -islandGrouping 7.5 -landSlope 7.0 -mountainRanges 5 -mountainSteepness 2.5 \
  #                 -seed 32
  #$parms configure -continents 7 -oceans 7 -largeIslands 2 -seas 3 -smallIslands 32 -lakes 128 \
  #                 -islandGrouping 10 -landSlope 10.0 -mountainRanges 5 -mountainSteepness 2.1 \
  #                 -seed 2
  #$parms configure -continents 0 -oceans 32 -largeIslands 0 -seas 0 -smallIslands 64 -lakes 64 \
  #                 -islandGrouping 0 -landSlope 6 -mountainRanges 2 -mountainSteepness 4 \
  #                 -enormousMountains 0 -seed 2
  $parms configure -continents 7 -oceans 0 -largeIslands 0 -seas 0 -smallIslands 32 -lakes 32 \
                   -islandGrouping 6 -landSlope 6 -mountainRanges 3 -mountainSteepness 3 \
                   -rivers 32 -enormousMountains 0 -craters 0 \
                   -equatorTemperature 313 -polarTemperature 263 -solarEquator 0.6 \
                   -waterTemperatureDelta 10 -altitudeTemperatureDelta -35 \
                   -freezingPoint 273.15 -vapourTransport 0.0625 -mountainBlockage 0.0625 -humidity 0 \
                   -vegitationHumidity 0.5 \
                   -vegitationColour [expr 0x088010] -waterColour [expr 0x101880] \
                   -lowerPlanetColour [expr 0xFFE060] -upperPlanetColour [expr 0xAAAAAA] \
                   -seed 2
  #$parms configure -continents 0 -oceans 1 -largeIslands 1 -seas 1 -smallIslands 512 -lakes 256 \
  #                 -islandGrouping 18 -landSlope 6 -mountainRanges 1 -mountainSteepness 3 \
  #                 -enormousMountains 2 -seed 2
  #$parms configure -continents 1 -oceans 0 -largeIslands 0 -seas 0 -smallIslands 0 -lakes 0 \
  #                 -landSlope 3 -mountainRanges 5 -mountainSteepness 3 -enormousMountains 5 \
  #                 -rivers 0 -craters 10 -maxCraterSize 0.4 \
  #                 -equatorTemperature 313 -polarTemperature 263 -solarEquator 0.6 \
  #                 -waterTemperatureDelta 10 -altitudeTemperatureDelta -25 \
  #                 -freezingPoint 273.15 -vapourTransport 1 -mountainBlockage 0.1 -humidity 0 \
  #                 -seed 0
  planetgen_begin $parms
  delete object $parms
}

# Convenience network test procs
proc ntl {args} {
  set ::state [new NetworkTest default]
  foreach {host port} $args {
    $::state connect $host $port
  }
}

proc ntc {args} {
  ntl 192.168.10.199 12544 {*}$args
}

source tcl/boot.tcl
set oldState $state
set state [new BootManager boot]
delete object $oldState
