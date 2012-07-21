# Allows for imposing certain behaviours on standard AI descriptions.
#
# output: Destination; must be an existing libconfig group; it will be emptied
#   of any contents already present.
# input: Source description (libconfig path) to weave
# aspects: Aspect list (libconfig path) to weave
# mode: A string used to select which aspects are applied
#
# Returns: A human-readable string indicating any diagnostics the developer may
#   be interested in.
#
# If the input is invalid, an error will be raised.
#
# Input for the aspect weaver comes in two pieces. The first is the standard AI
# description, which is like a normal AI description (see
# src/control/ai/aictrl.hxx), except that it MUST omit the "boot" state and
# MUST contain a "main" state. The "weight" field is optional in all cases and
# assumed to be 1 if omitted.
#
# The second input is a set of aspects to apply to the description. Each aspect
# defines the following:
#   A list of Tcl glob patterns used to determine whether to actually apply
#     the aspect;
#   An optional list of modules to run in the boot procedure before switching
#     to main;
#   An optional list of states to create;
#   A list of concerns to apply;
#   An optional list of Tcl glob patterns indicating modules not permitted in
#     affected states;
#   A boolean indicating whether it is authoritative.
#
# A concern consists of a list of Tcl globs to find the states to modify, and a
# set of modules to add to such states. These modules are specified the same
# way as in the standard AI descriptions, except the weight is a percentage
# (0..100); the percentage is relative to the total weight of the state AT THE
# TIME THE CONCERN IS APPLIED. This means that a weight of 100 will result in
# the concern's contents being called 50% of the time, assuming no other added
# modules. Percentages over 100 are legal.
#
# Concerns may also specify reflexes. Such reflexes will replace user reflexes
# if more than four slots are taken.
#
# The weaver imposes some restrictions on the AI descriptions: States which are
# not matched by any concern of an authoritative aspect are dropped; modules
# with a weight exceeding 8 are dropped; procedures with a total run length
# greater than 8 are dropped; modules which are refused by concerns are
# dropped; reflex slots needed by concerns are forcibly emptied.
#
# An aspect list is a libconfig group. Each aspect in the aspect list is a
# subgroup; its name is irrelevant. The contents of the subgroup are:
#   string array: modes
#     An array of Tcl globs matched against a provided mode string; the aspect
#     will be applied if any of the globs match.
#   concern group: concerns
#     A group of concerns to apply.
#   boolean: authoritative
#     If true, a state the aspect applies to is permitted.
#   module list: boot (optional)
#     A list of module descriptions to splice into the boot procedure.
#   state group: states (optional)
#     A group of state descriptions to splice into the AI description.
#   string array: forbidden (optional)
#     An array of Tcl globs used to check whether any source module is
#     permitted. Any module which matches any pattern is dropped.
#     This does not apply to modules spliced by this or other aspects.
#
# A concern is a subgroup within the "concerns" group. Its name is
# irrelevant. Its contents are:
#   string array: points
#     An array of Tcl globs to match against state names. If any match, this
#     concern is spliced into the state. Splicing will occur for all states
#     present, including states inserted by this and other aspects.
#   module list: advice
#     A list of modified module declarations (see main description) to splice
#     into the state's module list. The contents are not subject to
#     restrictions imposed by the weaver or any aspect, including this one.
#
# After weaving, all state and module declarations from the original input
# still have all fields from the original (barring items defined to be removed).
# This includes items at the root which do not appear to be valid states.
proc aspectaiWeave {output input aspectList mode} {
  set diagnostics {}

  # Empty the output location
  while {[$ length $output]} {
    $ remix $output 0
  }

  # Copy to destination
  confcpy $output $input

  # Select the aspects to apply
  set aspects {}
  conffor aspect $aspectList {
    conffor modeglob $aspect.modes {
      if {[string match [$ str $modeglob] $mode]} {
        lappend aspects $aspect
        break
      }
    }
  }

  # Remove any boot state, if it exists
  if {[$ exists $output.boot]} {
    append diagnostics "Invalid boot state deleted\n"
    $ remove $output.boot
  }

  # Create the boot state
  $ add $output boot STList
  $ append $output.boot STGroup
  $ adds $output.boot.\[0\] module "core/procedure"
  $ addi $output.boot.\[0\] weight 1
  $ add  $output.boot.\[0\] modules STList
  set boot $output.boot.\[0\].modules

  # Build list of forbidden module patterns
  set forbidden {}
  foreach aspect $aspects {
    if {[$ exists $aspect.forbidden]} {
      conffor fc $aspect.forbidden {
        lappend forbidden [$ str $fc]
      }
    }
  }

  # Remove forbidden modules and add implicit weights
  conffor sub $output {
    if {[$ getType $sub] == "STList"} {
      append diagnostics [aspectaiSanitise $sub $forbidden]
    }
  }

  # Ensure that all states have an authoritative match
  set unmatched {}
  conffor state $output {
    if {[$ getType $state] == "STList"} {
      set matched no
      set name [$ name $state]
      # Special exception for boot since it is managed by the weaver
      if {$name == "boot"} continue
      foreach aspect $aspects {
        if {$matched} break
        if {[$ bool $aspect.authoritative]} {
          conffor concern $aspect.concerns {
            if {$matched} break
            conffor pattern $concern.points {
              if {$matched} break
              set matched [string match [$ str $pattern] $name]
            } ;# End for pattern
          } ;# End for concern
        } ;# End if authoritative
      } ;# End for aspect

      if {!$matched} {
        append diagnostics \
"State $name was unmatched by any authoritative aspect and will be removed\n"
        lappend unmatched $state
      }
    } ;#End if is list
  } ;# End for state

  # Must remove in reverse order since items are based on indices.
  foreach state [lreverse $unmatched] {
    $ remove $state
  }

  # Add states introduced by the aspects
  foreach aspect $aspects {
    if {[$ exists $aspect.states]} {
      conffor state $aspect.states {
        set name [$ name $state]
        if {[$ exists $output.$name]} {
          append diagnostics "Replacing aspect-provided state $name\n"
          $ remove $output.$name
        }

        $ add $output $name STList
        confcpy $output.$name $state
        # Ensure weight is set
        conffor module $output.$name {
          if {![$ exists $module.weight]} {
            $ addi $module weight 1
          }
        }
      }
    }
  }

  # Weave concerns
  conffor state $output {
    if {[$ getType $state] == "STList"} {
      foreach aspect $aspects {
        conffor concern $aspect.concerns {
          set matches no
          conffor pattern $concern.points {
            if {[string match [$ str $pattern] [$ name $state]]} {
              set matches yes
              break
            }
          } ;# End for pattern

          # If no pattern matched, this state does not apply
          if {!$matches} continue

          # Calculate the total weight of the state as-is
          set total 0
          conffor module $state {
            incr total [$ int $module.weight]
          } ;# End for module

          # Add advice modules
          conffor advice $concern.advice {
            if {[$ exists $advice.weight]} {
              set weightPercent [$ int $advice.weight]
            } else {
              set weightPercent 1
            }
            set weight [expr {int(ceil($weightPercent/100.0*$total))}]

            set ix [$ length $state]
            $ append $state STGroup
            confcpy $state.\[$ix\] $advice
            if {[$ exists $state.\[$ix\].weight]} {
              $ seti $state.\[$ix\].weight $weight
            } else {
              $ addi $state.\[$ix\] weight $weight
            }
          } ;# End for advice
        } ;# End for concern
      } ;# End for aspect
    } ;# End if is list
  } ;# End for state

  # Remove overflowing reflexes
  conffor state $output {
    if {[$ getType $state] != "STList"} continue

    set reflexes {}
    conffor module $state {
      if {[$ exists $module.reflex]} {
        lappend reflexes $module
      }
    } ;# End for moudle

    # Reverse so removal will be in correct order and so that the first four
    # reflexes are the ones to keep (which will have come from aspects, if any
    # reflexes were added by them).
    set reflexes [lreverse $reflexes]
    foreach reflex [lrange $reflexes 4 end] {
      append diagnostics "Removing overflowing reflex $reflex"
      $ remove $reflex
    }
  } ;# End for state

  # Add aspect boot code
  foreach aspect $aspects {
    if {[$ exists $aspect.boot]} {
      conffor advice $aspect.boot {
        set ix [$ length $boot]
        $ append $boot STGroup
        confcpy $boot.\[$ix\] $advice
      } ;# End for advice
    } ;# End if has boot
  } ;# End for aspect

  # Add the final code to switch to main
  set ix [$ length $boot]
  $ append $boot STGroup
  set fin $boot.\[$ix\]
  $ adds $fin module "state/goto"
  $ adds $fin target "main"
  $ addi $fin weight 1

  return $diagnostics
}

# Helper proc for aspectaiWeave
# Removes forbidden modules and adds implicit weights where needed
proc aspectaiSanitise {src forbidden} {
  set diagnostics {}
  do i 0 "\[$ length {$src}\]" {
    if {![$ exists $src.\[$i\].weight]} {
      $ addi $src.\[$i\] weight 1
    }
    if {[$ int $src.\[$i\].weight] > 8} {
      append diagnostics "Removing module with weight > 8\n"
      $ remove $src.\[$i\]
      incr i -1
      continue
    }

    if {[$ exists $src.\[$i\].module]} {
      set mod [$ str $src.\[$i\].module]
      # Ensure that no reflex is present
      if {[$ exists $src.\[$i\].reflex]} {
        $ remove $src.\[$i\].reflex
      }
    } else {
      set mod [$ str $src.\[$i\].reflex]
    }
    set ok yes
    foreach forb $forbidden {
      if {[string match $forb $mod]} {
        append diagnostics "Removing forbidden module $mod\n"
        $ remove $src.\[$i\]
        incr i -1
        set ok no
        break
      }
    }

    if {!$ok} continue ;# Removed what we were working on

    # If it has child modules, recurse over those, and forbid procedures
    if {[$ exists $src.\[$i\].modules]} {
      append diagnostics \
          [aspectaiSanitise $src.\[$i\].modules \
               [list {*}$forbidden *procedure*]]

      # Remove if total weight exceeds 8
      set w 0
      conffor module $src.\[$i\].modules {
        incr w [$ int $module.weight]
      }

      if {$w > 8} {
        append diagnostics "Removing procedure longer than 8\n"
        $ remove $src.\[$i\]
        incr i -1
        continue
      }
    }
  }

  return $diagnostics
}
