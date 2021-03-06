# Loads all ship files and adds them to the appropriate
# core hangars (after loading the hangar file itself).
#
# Also provides the refreshStandardHangars and makeHangarEffective
# procs

$ open [homeq hangar/list.rc] hangar

set shipLoadQueue [glob -nocomplain [homeq hangar/*/*.ship]]
set numShipsToLoad [llength $shipLoadQueue]

# [Re]Populate standard "user" hangars
proc refreshStandardHangars {} {
  foreach {source} {c b a all_ships} {
    if {$source == "all_ships"} {
      set name [_ A misc class_all_ships]
    } else {
      set name [format [_ A misc all_class_x] [string toupper $source]]
    }
    if {[$ exists hangar.user.std_$source]} {
      $ remove hangar.user.std_$source
    }
    $ add hangar.user std_$source STGroup
    $ adds hangar.user.std_$source name $name
    $ addb hangar.user.std_$source deletable no
    $ add hangar.user.std_$source contents STList
    for {set i 0} {$i < [$ length hangar.$source]} {incr i} {
      $ append hangar.user.std_$source.contents STGroup
      set ship [$ str hangar.$source.\[$i\]]
      $ adds hangar.user.std_$source.contents.\[$i\] target $ship
      $ addi hangar.user.std_$source.contents.\[$i\] weight 1
    }
  }
}

# Makes a hangar effective, given an index of the
# user hangar to use
proc makeHangarEffective ix {
  makeHangarEffectiveByName hangar.user.\[$ix\]
}

proc makeHangarEffectiveByName hangar {
  while {[$ length hangar.effective]} {
    $ remix hangar.effective 0
  }
  for {set i 0} {$i < [$ length $hangar.contents]} {incr i} {
    for {set j 0} {$j < [$ int $hangar.contents.\[$i\].weight]} {incr j} {
      $ appends hangar.effective \
          [shipName2Mount [$ str $hangar.contents.\[$i\].target]]
    }
  }
}

# First, clear the core hangar lists
foreach hangar {all_ships a b c effective} {
  if {![$ exists hangar.$hangar]} {
    $ add hangar $hangar STArray
  } else {
    while {[$ length hangar.$hangar]} {
      $ remix hangar.$hangar 0
    }
  }

  # Make sure the user group exists
  if {![$ exists hangar.user]} {
    $ add hangar user STGroup
  }
}

$state setCallback [_ A boot ships] {
  if {[catch {
  # Load a few ships
  for {set i 0} {$i < 16 && [llength $shipLoadQueue]} {incr i} {
    set shipLoadQueue [lassign $shipLoadQueue path]
    set ship [shipPath2Mount $path]
    if {[catch {
      $ openLazily $path $ship
    } err]} {
      log "Couldn't load ship file $path: $err"
      continue
    }
    $ appends hangar.all_ships [shipMount2Name $ship]
    set cls [string tolower [spc::get $ship class]]
    # Make sure the class is valid
    switch -- $cls {
      a -
      b -
      c {
        $ appends hangar.$cls [shipMount2Name $ship]
      }
      default {
        log "Warning: $ship has invalid class $cls"
        continue
      }
    }

    # If we actually loaded the ship, don't load any more this frame
    if {[$ loaded $ship]} {
      break
    }
  }
  } err]} { log $err }

  if {[llength $shipLoadQueue]} {
    expr {($numShipsToLoad-[llength $shipLoadQueue])*100/$numShipsToLoad}
  } else {
    refreshStandardHangars
    # Ensure that the user has "preferred" ships
    if {![$ exists conf.preferred]} {
      $ add conf preferred STGroup
    }
    foreach class {A B C} {
      if {![$ exists conf.preferred.$class] ||
          ![$ exists [$ str conf.preferred.$class]]} {
        catch {
          $ remove conf.preferred.$class
        }
        $ adds conf.preferred $class \
            [shipName2Mount [$ str hangar.[string tolower $class].\[0\]]]
      }
    }

    if {![$ exists conf.preferred.main] ||
        ![$ exists [$ str conf.preferred.main]]} {
      catch {
        $ remove conf.preferred.main
      }
      $ adds conf.preferred main [$ str conf.preferred.C]
    }
    expr {200}
  }
}
