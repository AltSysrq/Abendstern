# Provides a simple GUI to run the game from, until
# we have normal game modes available
class TestApp {
  inherit gui::Application

  public variable retval

  constructor {} {Application::constructor} {
    global gameClass humanShip testStateMode testStateSize
    set retval 0
    set gameClass effective
    set humanShip 0
    set testStateMode TSMFreeForAll
    set testStateSize 100
    set mode [new TestMode $this]
  }

  method drawThis {} {
    ::sbg::draw
  }

  method updateThis et {
    ::sbg::update $et
    return $retval
  }

  method setSubState app {
    set subapp $app
  }

  method setMode md {
    set mode $md
  }
}

proc boolSetCheckbox {name setting} {
  new ::gui::Checkbox $name "\$ bool $setting" "\$ setb $setting"
}
proc setLowResTex enable {
  $ seti conf.graphics.low_res_textures [expr {$enable? 64:0}]
}

proc runCmd cmd {
  global commandBox commandResult
  if {[catch {
    $commandResult setText [namespace eval :: $cmd]
    $commandBox setText {}
  } err]} {
    $commandResult setText "[_ A gui error]: $err"
  }
}

proc setHumanShip ix {
  global shipSelector
  set ::humanShip $ix ;# TestState
  set ::humanShipMount [$ str hangar.effective.\[$ix\]] ;# Other
}

class TestMode {
  inherit ::gui::Mode
  variable app
  variable gameparms ;# So we can apply the parms without disturbing other settings

  method makeHangarEffective {} {
    ::makeHangarEffective [$::hangarList getSelection]
    set ::humanShip 0
    set ::humanShipMount [$ str hangar.effective.\[0\]]
    $::shipSelector reset hangar.effective
  }
  constructor a {
    ::gui::Mode::constructor
  } {
    set app $a
    set supermain [new gui::BorderContainer]
    set main [new gui::VerticalContainer 0.01]
    $main add [new gui::Button [_ T a begin_game] \
                   "$app setSubState \[new GameGUI\]"]
    $main add [new gui::Button [_ T a ship_mgmt] "$app setSubState \[new ShipEditor\]"]
    if {[string length $::abnet::userid]} {
      # Logged in to network, give appropriate options
      $main add [new gui::Button [_ T a acct_mgmt] \
        "$app setSubState \[new AccountManager\]"]
      $main add [new gui::Button [_ T a shipbrowse] \
        "$app configure -retval \[new ShipBrowser\]"]
    }
    $main add [new gui::Button [_ T a logout] \
      "$app configure -retval \[new BootManager logout\]"]
    if {"WINDOWS" == $::PLATFORM} {
      $main add [new gui::Button [_ T a update_abendstern] "$app setMode \[new SelfUpdater\]"]
    }
    set quit [new gui::Button [_ A gui quit] "$app configure -retval \[new BootManager shutdown\]"]
    $quit setCancel
    $main add $quit
    global commandBox commandResult
    set commandBox [new gui::TextField Tcl \
                    "" \
                    {} {} runCmd]
    set commandResult [new gui::MultiLineLabel]
    $main add $commandBox
    $supermain setElt top $main
    $supermain setElt centre $commandResult
    set tabs [new gui::TabPanel]
    set root [new gui::ComfyContainer [
      new gui::Frame $tabs
    ] 0.8 ]
    $tabs add [_ T a tab_main] $supermain
    # Produce list of hangar names
    set hangarNames {}
    for {set i 0} {$i < [$ length hangar.user]} {incr i} {
      lappend hangarNames [$ str hangar.user.\[$i\].name]
    }
    global hangarList
    set hangarList [new gui::List [_ T a choose_hangar] $hangarNames no "$this makeHangarEffective"]
    set hangarTab [new gui::BorderContainer 0.01]
    $hangarTab setElt centre $hangarList
    set hangarButtons [new gui::HorizontalContainer 0.02 centre]
    $hangarButtons add [new gui::Button [_ T a new_dots] [format {
      # Find the first free user slot
      for {set i 0} {[$ exists hangar.user.usr$i]} {incr i} {}
      $ add hangar.user usr$i STGroup
      $ addb hangar.user.usr$i deletable yes
      $ adds hangar.user.usr$i name [_ A gui untitled]
      $ add hangar.user.usr$i contents STList
      %s setMode [new gui::HangarEditor hangar.user.usr$i {
        %s setMode %s
        global hangarList
        # Update list
        # Note that the hangar might not exist, in which case we do not change
        # the effective hangar, and unselect everything in the list.
        if {[$ exists hangar.user.usr$i]} {
          set cts [$hangarList getItems]
          lappend cts [$ str hangar.user.usr$i.name]
          $hangarList setItems $cts
          $hangarList setSelection [expr {[llength $cts]-1}]
          %s makeHangarEffective
          $ sync hangar
        } else {
          $hangarList setSelection 0
          %s makeHangarEffective
        }
      }]
    } $app $app $this $this $this]]
    $hangarButtons add [new gui::Button [_ T a edit_dots] [format {
      global hangarList
      set sel [$hangarList getSelection]
      if {[llength $sel]} {
        # Make sure editable
        if {[$ bool hangar.user.\[$sel\].deletable]} {
          # Edit
          %s setMode [new gui::HangarEditor hangar.user.\[$sel\] {
            %s setMode %s
            # Update list (possible name change)
            set cts [$hangarList getItems]
            set cts [lreplace $cts $sel $sel [$ str hangar.user.\[$sel\].name]]
            $hangarList setItems $cts
            $hangarList setSelection $sel
            %s makeHangarEffective
            $ sync hangar
          }]
        }
      }
    } $app $app $this $this]]
    $hangarButtons add [new gui::Button [_ T a delete] [format {
      global hangarList
      set sel [$hangarList getSelection]
      if {[llength $sel]} {
        # Make sure deletable
        if {[$ bool hangar.user.\[$sel\].deletable]} {
          # Remove
          set cts [$hangarList getItems]
          set cts [lreplace $cts $sel $sel]
          $hangarList setItems $cts
          $hangarList setSelection 0
          %s makeHangarEffective

          # Remove from actual hangar
          $ remix hangar.user $sel
          $ sync hangar
        }
      }
    } $this]]
    $hangarTab setElt bottom $hangarButtons
    $hangarList setSelection 0
    $tabs add [_ T a tab_hangar] $hangarTab
    global shipSelector
    set shipSelector [new gui::ShipChooser hangar.all_ships setHumanShip]
    makeHangarEffective
    $tabs add [_ T a tab_ship] $shipSelector
    set settingsTop [new gui::BorderContainer 0 0.02]
    set settingsButtons [new gui::HorizontalContainer 0.01 right]
    $settingsButtons add [new gui::Button [_ T a save] "$root save; $ sync conf"]
    $settingsButtons add [new gui::Button [_ T a revert] "$ revert conf; $root revert"]
    $settingsTop setElt bottom $settingsButtons
    set settings [new gui::TabPanel]
    set gameSettings [new gui::VerticalContainer 0.01]
    $gameSettings add [new gui::Label "[_ T a ship_colour]:"]
    foreach {ix comp} {0 {Red  } 1 {Green} 2 {Blue }} {
      $gameSettings add [new gui::Slider $comp float \
                  "$ float {conf.ship_colour.\[$ix\]}" \
                  "$ setf {conf.ship_colour.\[$ix\]}" \
                  0.0 1.0 0.05]
    }
    $settings add [_ T a settings_game] $gameSettings
    set graphSettings [new gui::VerticalContainer 0.01]
    $graphSettings add [boolSetCheckbox [_ T a antialiasing] conf.graphics.antialiasing]
    $graphSettings add [boolSetCheckbox [_ T a full_alpha_blending] conf.graphics.full_alpha_blending]
    $graphSettings add [boolSetCheckbox [_ T a any_alpha_blending] conf.graphics.any_alpha_blending]
    $graphSettings add [boolSetCheckbox [_ T a smooth_scaling] conf.graphics.smooth_scaling]
    $graphSettings add [new gui::Checkbox [_ T a low_res_textures] \
                        {$ int conf.graphics.low_res_textures} \
                        setLowResTex]
    $graphSettings add [boolSetCheckbox [_ T a high_quality] conf.graphics.high_quality]
    $graphSettings add [boolSetCheckbox [_ T a show_fps] conf.hud.show_fps]
    $graphSettings add [new gui::Slider [_ T a font_size] float \
                        {$ float conf.hud.font_size} \
                        {$ setf conf.hud.font_size} \
                        0.01 0.05 0.005]
    $graphSettings add [boolSetCheckbox [_ T a fullscreen] conf.display.fullscreen]
    $settings add [_ T a settings_graphics] $graphSettings
    set langSetting [new gui::VerticalContainer 0.01]
    set prevbox none
    set f [open data/lang/manifest r]
    set languageInfo [read $f]
    close $f
    foreach {lang name} $languageInfo {
      set box [new gui::RadioButton $name "expr {{$lang} == \[$ str conf.language\]}" \
                "$ sets conf.language $lang" $prevbox]
      set prevbox $box
      $langSetting add $box
    }
    $settings add [_ T a settings_language] $langSetting
    set controlSettings [new gui::VerticalContainer 0.01];
    set prevbox none
    for {set i 0} {$i < [$ length conf]} {incr i} {
      if {-1 != [string first _control [$ name conf.\[$i\]]]} {
        set name [$ name conf.\[$i\]]
        set cmnt [$ str conf.$name.comment]
        if {[string length $cmnt] && "*" == [string index $cmnt 0]} {
          set cmnt [_ A control_scheme $cmnt]
        }
        set box [new gui::RadioButton $cmnt \
                  "expr {{$name} == \[$ str conf.control_scheme\]}" \
                  "$ sets conf.control_scheme $name" $prevbox]
        set prevbox $box
        $controlSettings add $box
      }
    }
    $controlSettings add [new ::gui::Button [_ A ctrledit edit_custom_dots] \
                              "$app setSubState \[new ControlEditor\]"]
    $settings add [_ T a settings_controls] $controlSettings
    set colourSettings [new gui::TabPanel]
    foreach colour {Standard Warning Danger Special} {
      set ckey [string tolower $colour]
      set panel [new gui::VerticalContainer 0.01]
      foreach {ix comp} {0 red 1 green 2 blue 3 opacity} {
        $panel add [new gui::Slider [_ T a $comp] float \
                    "$ float {conf.hud.colours.$ckey.\[$ix\]}" \
                    "$ setf {conf.hud.colours.$ckey.\[$ix\]}" \
                    0.0 1.0 0.05]
      }
      $colourSettings add [_ T a $colour] $panel
    }
    $settings add [_ T a settings_colours] $colourSettings
    $settingsTop setElt centre $settings
    $settingsTop makeCentreLast
    $tabs add [_ T a tab_settings] $settingsTop
    refreshAccelerators
    global vheight
    $root setSize 0 1 $vheight 0
  }
}
