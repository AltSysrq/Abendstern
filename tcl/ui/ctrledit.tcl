# Contains classes that allow the user to configure the control system.

# Digital actions.
# This is a dict keyed to the l10n entry (which is the same as the action used
# by hc_conf).  Contents
# are:
#   repeat    Boolean
#   value     "int", "float", or "enum" if the value is used, not present if
#             value is not used
#   valmin    Minimum value, only included if value is numeric
#   valmax    Maximum value, only included if value is numeric
#   valdef    Default value, only included if value is numeric
#   valopt    Options for enumerated value; first is default.
#             Any two-element list uses the first elt as the value, and the
#             second for l10n.
#   enuml10ns The l10n section to use to look the enum name up in
#   enuml10np The prefix to give to each enum value when looking up
#   note      l10n entry for additional note, if required
#   hidden    If specified, the user cannot choose this option
set DIGITAL_ACTIONS {
  accel     { repeat 0 }
  decel     { repeat 0 }
  stats     { repeat 0 }
  throttle  { repeat 1 value float valmin -0.1 valmax +0.1 valdef 0.05
              note throttle_note }
  rotate    { repeat 1 value float valmin -0.004 valmax +0.004 valdef +0.004
              note rotate_note }
  fire      { repeat 1 }
  compose   { repeat 0 }
  "adjust weapon power" {repeat 1 value int valmin -8 valmax +8 valdef +1 }
  "set current weapon" {
    repeat 0
    value enum
    valopt {
      { EnergyChargeLauncher energy_charge_launcher_s }
      { MagnetoBombLauncher magneto_bomb_launcher_s }
      { PlasmaBurstLauncher plasma_burst_launcher_s }
      { SGBombLauncher semiguided_bomb_launcher_s }
      { GatlingPlasmaBurstLauncher gatling_plasma_burst_launcher_s }
      { MonophasicEnergyEmitter monophasic_energy_emitter_s }
      { MissileLauncher missile_launcher_s }
      { ParticleBeamLauncher particle_beam_launcher_s }
    }
    enuml10ns sys
    enuml10np {}
  }
  "next weapon" { repeat 0 }
  "prev weapon" { repeat 0 }
  "show fullscreen map" { repeat 0 }
  "set camera mode" {
    repeat 0
    value enum
    valopt { none rotation velocity }
    enuml10ns ctrledit
    enuml10np cameramode_
  }
  "adjust camera lookahead" {
    repeat 1
    value float
    valmin -0.02
    valmax +0.02
    valdef +0.01
    note lookahead_note
  }
  stealth { repeat 0 }
  "self destruct" { repeat 0 }
  retarget { repeat 0 }
  "adjust camera zoom" {
    repeat 1
    value float valmin -0.5 valmax +0.5 valdef +0.1
    note zoom_note
  }
  "__ exit" { repeat 0 }
  "__ frameXframe" { repeat 0 hidden 1 }
  "__ fast" { repeat 0 hidden 1 }
  "__ halt" { repeat 0 hidden 1 }
  "__ slow" { repeat 0 hidden 1 }
  "__ unbound" { repeat 0 hidden 1 }
}

# Allows the user to edit the conf.custom_control control schema (or another).
class ControlEditor {
  inherit ::gui::Application

  # The root of the config schema to edit
  variable root
  variable ret

  constructor {{schema conf.custom_control}} {
    ::gui::Application::constructor
  } {
    set root $schema
    set ret 0
    set mode [new ControlEditorDeviceSelector $this]
  }

  destructor {
    $ sync conf
  }

  method setMode m {
    lappend ::gui::autodelete $mode
    set mode $m
  }

  method die {} {
    $ sync conf
    set ret $this
  }

  method updateThis et {
    ::sbg::update $et
    $mode update $et
    return $ret
  }

  method drawThis {} {
    ::sbg::draw
  }

  method getRoot {} {
    return $root
  }

  method getCursor {} {
    if {![$mode isBusy]} {
      return crosshair
    } else {
      return busy
    }
  }

  # The only time we're busy is when waiting on a key
  method disableKeyboardInput {} {
    return 0
  }
}

# The GUI for the user to select a device to edit, or to exit the editor.
class ControlEditorDeviceSelector {
  inherit ::gui::Mode

  variable app

  constructor {_app} {
    ::gui::Mode::constructor
  } {
    set app $_app

    set main [new ::gui::VerticalContainer 0.01]
    $main add [new ::gui::Label [_ A ctrledit title] centre]
    $main add [new ::gui::Label [_ A ctrledit select_device] left]
    $main add [new ::gui::Button [_ A ctrledit keyboard] "$this editkbd"]
    $main add [new ::gui::Button [_ A ctrledit mouse] "$this editmouse"]
    for {set i 0} {$i < [joystick_count]} {incr i} {
      $main add [new ::gui::Button [joystick_name $i] "$this editjoy $i"]
    }

    set can [new ::gui::Button [_ A gui quit] "$app die"]
    $can setCancel
    $main add $can

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method editkbd {} {
    $app setMode [new ControlEditorKeyboardConfigurator $app]
  }

  method editmouse {} {
    $app setMode [new ControlEditorMouseConfigurator $app]
  }

  method editjoy ix {
    $app setMode [new ControlEditorJoystickConfigurator $app $ix]
  }

  method isBusy {} {
    return no
  }
}

# Common base class for all configurators.
# Each device provides a list of digital roots and their human-readable names,
# as well as analogue roots and names. There is special handling available for
# the keyboard configurator to have the user press a key and add that key.
class ControlEditorBaseConfigurator {
  inherit ::gui::Mode

  variable lstdigitalActions ;# The list of digital actions and their bindings
  variable lstanalogueActions ;# The list of analogue axes and their bindings
  variable lstdigitalTypes ;# List of digital actions that can be selected
  variable pdigitalActionValue ;# The panel containing the digital action
                                # value, if any
  variable ranalogueNone
  variable ranalogueRotation
  variable ranalogueThrottle
  variable ranalogueAccel
  variable sanalogueSensitivity ;# Slider for analogue axis sensitivity
  variable canalogueJoystick ;# Whether the given analogue axis should operate
                              # in "joystick mode"
  variable canalogueInvert ;# Whether to invert the given analogue axis

  # If true, the keyboard configurator should handle the next keyboard event
  # specially by creating a new unbound entry and calling the keyboardRefresh
  # method.
  protected variable keyboardInterceptNext no

  variable needRefresh no

  variable app

  constructor {app_ title isKeyboard} {
    set app $app_

    set top [new ::gui::BorderContainer]
    set close [new ::gui::Button [_ A gui close] "$this close"]
    $close setCancel
    set pclose [new ::gui::HorizontalContainer 0.01 right]
    $pclose add $close
    $top setElt bottom $pclose
    $top setElt top [new ::gui::Label $title centre]

    set main [new ::gui::TabPanel]
    set pdig [new ::gui::HorizontalContainer 0.01 grid]
    set pdigmain [new ::gui::BorderContainer 0 0.01]
    set pdigsel [new ::gui::BorderContainer 0 0.01]
    set lstdigitalActions [new ::gui::List [_ A ctrledit buttons] {} no \
                               "$this digitalSelected"]
    # Build list of possible bindings; each is "human name\a\ainternal name"
    set items {}
    dict for {type parms} $::DIGITAL_ACTIONS {
      if {[dict exists $parms hidden]} continue
      lappend items "[_ A ctrledit c_$type]\a\a$type"
    }
    set items [lsort -dictionary $items]
    set lstdigitalTypes [new ::gui::List [_ A ctrledit dig_bindings] $items no \
                             "$this bindingSelected"]
    $pdigmain setElt centre $lstdigitalActions
    $pdigsel setElt centre $lstdigitalTypes
    set pdigitalActionValue [new ::gui::BorderContainer]
    # Nothing added for now (this happens when the selection is changed)
    $pdigsel setElt bottom $pdigitalActionValue
    $pdig add $pdigmain
    $pdig add $pdigsel
    set pdigbuttons [new ::gui::VerticalContainer 0.01 centre]
    # For keyboard, add "Add key" and "Remove key" buttons
    if {$isKeyboard} {
      $pdigbuttons add [new ::gui::Button [_ A ctrledit add_key] "$this addKey"]
      $pdigbuttons add [new ::gui::Button [_ A ctrledit del_key] "$this delKey"]
    }
    $pdigmain setElt bottom $pdigbuttons
    $main add [_ A ctrledit buttons] $pdig

    # If not the keyboard, add axes
    if {!$isKeyboard} {
      set pana [new ::gui::BorderContainer 0 0.01]
      set lstanalogueActions [new ::gui::List [_ A ctrledit axes] {} no \
                                  "$this analogueSelected"]
      $pana setElt centre $lstanalogueActions
      set panactrl [new ::gui::VerticalContainer 0.01]
      set ranalogueNone [new ::gui::RadioButton [_ A ctrledit ana_none] \
                             {} {} none "$this setAnalogue none"]
      $panactrl add $ranalogueNone
      set ranalogueRotation [new ::gui::RadioButton [_ A ctrledit ana_rotate] \
                                 {} {} $ranalogueNone \
                                 "$this setAnalogue rotate"]
      $panactrl add $ranalogueRotation
      set ranalogueThrottle [new ::gui::RadioButton [_ A ctrledit ana_throttle]\
                                 {} {} $ranalogueRotation \
                                 "$this setAnalogue throttle"]
      $panactrl add $ranalogueThrottle
      set ranalogueAccel [new ::gui::RadioButton [_ A ctrledit ana_accel] \
                              {} {} $ranalogueThrottle \
                              "$this setAnalogue accel"]
      $panactrl add $ranalogueAccel
      set sanalogueSensitivity [new ::gui::Slider [_ A ctrledit sensitivity] \
                                    float {expr 2} {} 0 4 0.1 \
                                    "$this adjustSensitivity"]
      $panactrl add $sanalogueSensitivity
      set canalogueJoystick [new ::gui::Checkbox [_ A ctrledit joystick_mode] \
                                 {} {} "$this setJoystickMode 1" \
                                 "$this setJoystickMode 0"]
      $panactrl add $canalogueJoystick
      set canalogueInvert [new ::gui::Checkbox [_ A ctrledit invert_axis] \
                               {} {} "$this setInvert 1" \
                               "$this setInvert 0"]
      $panactrl add $canalogueInvert
      $pana setElt bottom $panactrl
      $main add [_ A ctrledit axes] $pana
    } else {
      set lstanalogueActions 0
    }

    $top setElt centre $main
    set root [new ::gui::ComfyContainer [new ::gui::Frame $top]]
    refreshRoot
  }

  # Must be overridden by subclass.
  # Returns a list of digital event items. Each item is a two-element list; the
  # first element is the localised name of the control; the second is the root
  # of the configuration for that control (eg,
  # conf.custom_control.keyboard.key032), which must exist.
  method listDigitalControls {}
  # Must be overridden my subclass (other than keyboard).
  # Returns a list if analogue axes. Each axis is a four-element list. The
  # first element is the localised name of the control. The second is the
  # default value for joystick mode; the third indicates whether the user may
  # alter the value of joystick mode (eg, joystick axes always use joystick
  # mode, and joystick balls never do). The last is the config root of the
  # entry (eg, conf.custom_control.analogue.horiz), which must exist.
  method listAnalogueAxes {}

  method refreshRoot {} {
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method needRefresh {} {
    set needRefresh yes
  }

  method update et {
    if {$needRefresh} {
      refresh
      set needRefresh no
    }
  }

  method refresh {} {
    set digital [listDigitalControls]
    set elts {}
    foreach elt $digital {
      lassign $elt name confroot
      lappend elts [format "%s: %s\a\a%s" \
                        $name \
                        [formatAction $confroot] \
                        $confroot]
    }
    set oldsel [$lstdigitalActions getSelection]
    $lstdigitalActions setItems $elts
    $lstdigitalActions setSelection $oldsel

    if {$lstanalogueActions ne 0} {
      set analogue [listAnalogueAxes]
      set elts {}
      foreach elt $analogue {
        lassign $elt name joystick allowChangeJoystick confroot
        lappend elts [format "%s: %s\a\a%s" \
                          $name \
                          [formatAxis $confroot] \
                          [list $joystick $allowChangeJoystick $confroot]]
      }
      set oldsel [$lstanalogueActions getSelection]
      $lstanalogueActions setItems $elts
      $lstanalogueActions setSelection $oldsel
    }
  }

  method formatAction {root} {
    global DIGITAL_ACTIONS
    set type [$ str $root.action]
    if {![dict exists $DIGITAL_ACTIONS $type]} {
      return "????"
    }
    set name [_ A ctrledit c_$type]
    set parms [dict get $DIGITAL_ACTIONS $type]
    if {![dict exists $parms value] ||
        ![$ exists $root.value]} {
      return $name
    }

    if {"int" eq [dict get $parms value]} {
      return [format "%s %+d" $name [$ int $root.value]]
    } elseif {"float" eq [dict get $parms value]} {
      return [string trim [format "%s %+f" $name [$ float $root.value]] 0]
    } else {
      set opt "????"
      # Find the option selected
      set sel [$ str $root.value]
      foreach item [dict get $parms valopt] {
        if {2 == [llength $item]} {
          lassign $item internal l10n
        } else {
          set internal [set l10n [lindex $item 0]]
        }
        if {$internal eq $sel} {
          set opt [_ A [dict get $parms enuml10ns] \
                       "[dict get $parms enuml10np]$l10n"]
          break
        }
      }
      return [format "%s: %s" $name $opt]
    }
  }

  method formatAxis {confroot} {
    _ A ctrledit "ana_[$ str $confroot.action]"
  }

  method digitalSelected {} {
    if {{} == [$lstdigitalActions getSelection]} {
      $lstdigitalTypes setSelection {}
    } else {
      # Find the currently-bound item
      set sel {}
      set items [$lstdigitalTypes getItems]
      set confroot [lindex [$lstdigitalActions getItems] \
                           [$lstdigitalActions getSelection]]
      set confroot [extract $confroot]
      set action [$ str $confroot.action]

      for {set i 0} {$i < [llength $items]} {incr i} {
        if {[string match "*\a\a$action" [lindex $items $i]]} {
          set sel $i
          $lstdigitalTypes scrollTo $i
          break
        }
      }

      $lstdigitalTypes setSelection $sel
    }
    bindingSelected
  }

  method bindingSelected {} {
    set keySel [$lstdigitalActions getSelection]
    set actSel [$lstdigitalTypes getSelection]
    # If no binding is selected, no action may be selected
    if {$keySel == {} && $actSel != {}} {
      $lstdigitalTypes setSelection {}
      set actSel {}
    }

    # If no action is selected, clear the parms widget and we're done
    if {$actSel == {}} {
      $pdigitalActionValue setElt centre [new ::gui::AWidget]
      refreshRoot
      return
    }

    # At this point, we know that there is both a key and action selection
    set confroot [lindex [$lstdigitalActions getItems] $keySel]
    set action [lindex [$lstdigitalTypes getItems] $actSel]
    set confroot [extract $confroot]
    set action [extract $action]
    set parms [dict get $::DIGITAL_ACTIONS $action]

    # If the current action does not match what is selected, change it and set
    # to the default value.
    if {[$ str $confroot.action] ne $action} {
      while {[$ length $confroot]} {
        $ remix $confroot 0
      }

      $ adds $confroot action $action
      $ addb $confroot repeat [dict get $parms repeat]
      if {[dict exists $parms value]} {
        switch [dict get $parms value] {
          int {
            set meth addi
            set valdef [dict get $parms valdef]
          }
          float {
            set meth addf
            set valdef [dict get $parms valdef]
          }
          enum {
            set meth adds
            set valdef [lindex [dict get $parms valopt] 0 0]
          }
        }
        $ $meth $confroot value $valdef
      }

      # This changes the display of bindings
      needRefresh
    }

    # Create the value panel
    set vp [new ::gui::VerticalContainer 0.01]
    if {[dict exists $parms note]} {
      $vp add [new ::gui::Label [_ A ctrledit [dict get $parms note]] left]
    }
    if {[dict exists $parms value]} {
      switch -exact -- [dict get $parms value] {
        int -
        float {
          set t [dict get $parms value]
          if {$t == "int"} {
            set increment 1
            set setmeth seti
          } else {
            set increment [expr {([dict get $parms valmax]-
                                  [dict get $parms valmin])/10.0}]
            set setmeth setf
          }
          $vp add [new ::gui::Slider \
                       [_ A ctrledit value] $t \
                       [list expr [$ $t $confroot.value]] \
                       {} \
                       [dict get $parms valmin] \
                       [dict get $parms valmax] \
                       $increment \
                       "$this adjustValue $confroot $setmeth"]
        }
        enum {
          set prevbox none
          foreach item [dict get $parms valopt] {
            if {2 == [llength $item]} {
              lassign $item internal l10n
            } else {
              set l10n [set internal [lindex $item 0]]
            }
            set box [new ::gui::RadioButton \
                         [_ A [dict get $parms enuml10ns] \
                              "[dict get $parms enuml10np]$l10n"] \
                         {} {} $prevbox \
                         [list $this setValueStr $confroot $internal]]
            if {$internal eq [$ str $confroot.value]} {
              $box forceChecked 1
            }
            set prevbox $box
            $vp add $box
          }
        }
      }
    }

    $pdigitalActionValue setElt centre $vp
    refreshRoot
  }

  # Extracts the internal data from a human\a\ainternal string.
  method extract {from} {
    string range $from [string first "\a\a" $from]+2 end
  }

  # Creates an unbound digital entry at the given location if it does not
  # exist.
  method ensureDigitalExists {base sub} {
    if {![$ exists $base.$sub]} {
      $ add $base $sub STGroup
      $ adds $base.$sub action "__ unbound"
      $ addb $base.$sub repeat 0
    }
  }

  # Creates an unbound analogue entry at the given location if it does not
  # exist.
  method ensureAnalogueExists {base sub joystickMode} {
    if {![$ exists $base.$sub]} {
      $ add $base $sub STGroup
      $ adds $base.$sub action "none"
      $ addb $base.$sub recentre [expr {!$joystickMode}]
      $ addf $base.$sub sensitivity 1.0
    }
  }

  method adjustValue {confroot meth value} {
    $ $meth $confroot.value $value
    needRefresh
  }

  method setValueStr {confroot value} {
    $ sets $confroot.value $value
    needRefresh
  }

  method addKey {} {
    set keyboardInterceptNext 1
  }

  method isBusy {} {
    return $keyboardInterceptNext
  }

  method delKey {} {
    set sel [$lstdigitalActions getSelection]
    if {$sel == {}} return
    $ remove [extract [lindex [$lstdigitalActions getItems] $sel]]
    refresh
    digitalSelected
  }

  # Selects the digital action whose config root is given
  method selectKey {confroot} {
    set pattern "*\a\a$confroot"
    set items [$lstdigitalActions getItems]
    for {set i 0} {$i < [llength $items]} {incr i} {
      if {[string match $pattern [lindex $items $i]]} {
        $lstdigitalActions setSelection $i
        $lstdigitalActions scrollTo $i
        digitalSelected
        return
      }
    }
  }

  method analogueSelected {} {
    set sel [$lstanalogueActions getSelection]
    if {{} == $sel} {
      # Clear all radio boxen
      foreach box [list $ranalogueThrottle $ranalogueRotation $ranalogueNone] {
        $box forceChecked no
      }
      # Clear invert and joystick mode
      $canalogueInvert setCheckedNoAction no
      $canalogueJoystick setCheckedNoAction no
    } else {
      # Find what this is bound to and set all boxen appropriately
      lassign [extract [lindex [$lstanalogueActions getItems] $sel]] \
          defaultJoystick allowChangeJoystick confroot
      set curr [$ str $confroot.action]
      $ranalogueNone forceChecked [expr {"none" == $curr}]
      $ranalogueRotation forceChecked [expr {"rotate" == $curr}]
      $ranalogueThrottle forceChecked [expr {"throttle" == $curr}]
      $ranalogueAccel forceChecked [expr {"accel" == $curr}]
      $canalogueJoystick setCheckedNoAction \
          [expr {![$ bool $confroot.recentre]}]
      $canalogueInvert setCheckedNoAction \
          [expr {[$ float $confroot.sensitivity] < 0}]
      $sanalogueSensitivity setValue \
          [expr {abs([$ float $confroot.sensitivity])}]
    }
  }

  method adjustSensitivity {amt} {
    set sel [$lstanalogueActions getSelection]
    if {$sel == {}} return

    lassign [extract [lindex [$lstanalogueActions getItems] $sel]] \
        defaultJoystick allowChangeJoystick confroot

    if {[$canalogueInvert isChecked]} {
      set amt [expr {-$amt}]
    }

    $ setf $confroot.sensitivity $amt
  }

  method setAnalogue what {
    set sel [$lstanalogueActions getSelection]
    if {$sel == {}} {
      # Clear all and return
      analogueSelected
      return
    }

    lassign [extract [lindex [$lstanalogueActions getItems] $sel]] \
        defaultJoystick allowChangeJoystick confroot

    $ sets $confroot.action $what
    refresh
  }

  method setInvert {inverted} {
    set sel [$lstanalogueActions getSelection]
    if {{} == $sel} {
      # Uncheck the box
      $canalogueInvert setCheckedNoAction no
      return
    }

    # Readjusting the sensitivity will do the job
    adjustSensitivity [$sanalogueSensitivity getValue]
  }

  method setJoystickMode {jm} {
    set sel [$lstanalogueActions getSelection]
    if {{} == $sel} return

    lassign [extract [lindex [$lstanalogueActions getItems] $sel]] \
        defaultJoystick allowChangeJoystick confroot

    if {!$allowChangeJoystick} {
      # Set it back
      $canalogueJoystick setCheckedNoAction $defaultJoystick
      set jm $defaultJoystick
    }

    # Write
    $ setb $confroot.recentre [expr {!$jm}]
  }

  method close {} {
    $app setMode [new ControlEditorDeviceSelector $app]
  }
}

class ControlEditorKeyboardConfigurator {
  inherit ControlEditorBaseConfigurator

  variable confroot
  variable ignoreNextCharacterEvent no

  constructor {app} {
    ControlEditorBaseConfigurator::constructor $app [_ A ctrledit keyboard] 1
  } {
    set confroot [$app getRoot].keyboard
    refresh
  }

  method listDigitalControls {} {
    set lst {}
    for {set i 0} {$i < $::SDLK_LAST} {incr i} {
      set item [format "%s.key_%03d" $confroot $i]
      if {[$ exists $item]} {
        lappend lst [list [string totitle [SDL_GetKeyName $i]] $item]
      }
    }

    return $lst
  }

  method character args {
    # When capturing a key, the keyboard event (from which we get the keysym)
    # is sent before the character (used for buttons, etc), so we must ignore
    # the first character after a captured keyboard event.
    if {$ignoreNextCharacterEvent} {
      set ignoreNextCharacterEvent no
      return
    }

    chain {*}$args
  }

  method keyboard keyspec {
    if {!$keyboardInterceptNext} {
      chain $keyspec
      return
    }

    if {![string match "DOWN:????:*" $keyspec]} return

    set keyboardInterceptNext no
    set ignoreNextCharacterEvent yes

    set key [string range $keyspec [string length "DOWN:????:"] end]
    set key [SDLKeyToInt $key]
    set entry [format "key_%03d" $key]
    if {![$ exists $confroot.$entry]} {
      $ add $confroot $entry STGroup
      $ adds $confroot.$entry action "__ unbound"
      $ adds $confroot.$entry repeat 1
      refresh
    }
    selectKey $confroot.$entry
  }
}

class ControlEditorMouseConfigurator {
  inherit ControlEditorBaseConfigurator

  variable confroot

  constructor {app} {
    ControlEditorBaseConfigurator::constructor $app [_ A ctrledit mouse] no
  } {
    set confroot [$app getRoot]
    refresh
  }

  destructor {
    # Axes unbound must not be in joystick mode, or the cursor will be shown
    # in-game.
    if {"none" == [$ str $confroot.analogue.horiz.action] &&
        ![$ bool $confroot.analogue.horiz.recentre]} {
      $ setb $confroot.analogue.horiz.recentre yes
    }
    if {"none" == [$ str $confroot.analogue.vert.action] &&
        ![$ bool $confroot.analogue.vert.recentre]} {
      $ setb $confroot.analogue.vert.recentre yes
    }
  }

  method listDigitalControls {} {
    set lst {}
    foreach button {left middle right up down x1 x2} {
      set conf $confroot.mouse.$button
      # Create if it doesn't exist
      ensureDigitalExists $confroot.mouse $button
      lappend lst [list [_ A ctrledit mousebutton_$button] $conf]
    }
    return $lst
  }

  method listAnalogueAxes {} {
    list \
        [list [_ A ctrledit mouse_horiz] 0 1 $confroot.analogue.horiz] \
        [list [_ A ctrledit mouse_vert]  0 1 $confroot.analogue.vert ]
  }
}

class ControlEditorJoystickConfigurator {
  inherit ControlEditorBaseConfigurator

  variable confroot
  variable joyix

  constructor {app ix} {
    ControlEditorBaseConfigurator::constructor $app [joystick_name $ix]  no
  } {
    set confbase [$app getRoot]
    set joyix $ix
    set confroot $confbase.joy_$joyix
    # Create the joystick entry if it doesn't exist
    if {![$ exists $confroot]} {
      $ add $confbase joy_$joyix STGroup
    }

    refresh
  }

  method listDigitalControls {} {
    set lst {}
    # Conventional buttons
    for {set i 0} {$i < [joystick_buttonCount $joyix Button]} {incr i} {
      # Create if it doesn't exist
      ensureDigitalExists $confroot button_$i
      lappend lst [list [format [_ A ctrledit joystick_button] $i] \
                       $confroot.button_$i]
    }

    # Hats
    for {set i 0} {$i < [joystick_buttonCount $joyix HatUp]} {incr i} {
      foreach hat {up down left right} {
        ensureDigitalExists $confroot "hat_${hat}_$i"
        lappend lst [list [format [_ A ctrledit joystick_hat_$hat] $i] \
                         "$contfroot.hat_${hat}_$i"]
      }
    }

    return $lst
  }

  method listAnalogueAxes {} {
    set lst {}

    # Conventional axes
    for {set i 0} {$i < [joystick_axisCount $joyix Axis]} {incr i} {
      ensureAnalogueExists $confroot axis_$i 1
      # Axes 0 and 1 are named specially
      switch $i {
        0 { set name [_ A ctrledit joystick_x_axis] }
        1 { set name [_ A ctrledit joystick_y_axis] }
        default { set name [format [_ A ctrledit joystick_other_axis] $i] }
      }
      # It does not make sense to use non-joystick mode with joystick axes,
      # since they do not have infinite range.
      lappend lst [list $name 1 0 $confroot.axis_$i]
    }

    # Balls
    for {set i 0} {$i < [joystick_axisCount $joyix BallX]} {incr i} {
      foreach axis {x y} {
        set entry "ball_${axis}_$i"
        ensureAnalogueExists $confroot $entry 0
        set name [format [_ A ctrledit joystick_ball_$axis] $i]
        # We can't support joystick mode with balls, since they do not have
        # absolute coordinates.
        lappend lst [list $name 0 0 $confroot.$entry]
      }
    }

    return $lst
  }
}