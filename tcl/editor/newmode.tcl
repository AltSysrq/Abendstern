# Mode to allow the user to create a new ship in the ship editor

class ShipEditorNewShip {
  inherit ::gui::Mode

  variable editor
  variable nameField
  variable shipClass
  variable bridgeType

  constructor edr {
    ::gui::Mode::constructor
  } {
    set editor $edr
    set shipClass C
    set bridgeType square

    set main [new ::gui::BorderContainer 0.02 0.02]
    $main setElt top [new ::gui::Label [_ A editor newmode_title]]
    set controls [new ::gui::VerticalContainer 0.01]
    set nameField [new ::gui::TextField [_ A editor ship_name] {}]
    $controls add $nameField
    set radios [new ::gui::HorizontalContainer 0.1 centre]
    set leftRadios [new ::gui::VerticalContainer 0.01]
    $leftRadios add [new ::gui::Label [_ A editor ship_class] left]
    set rightRadios [new ::gui::VerticalContainer 0.01]
    $rightRadios add [new ::gui::Label [_ A editor bridge_type] left]
    set prevbox none
    foreach class {A B C} {
      set box [new ::gui::RadioButton "[_ A editor class_prefix]$class" "expr {[expr {$class == "C"}]}" {} \
               $prevbox "$this setClass $class"]
      $leftRadios add $box
      set prevbox $box
    }
    set prevbox none
    foreach {name type} [list circle circle square square equilt equil] {
      set box [new ::gui::RadioButton [_ A cell $name] "expr {[expr {$type == "square"}]}" {} \
               $prevbox "$this setBridge $type"]
      $rightRadios add $box
      set prevbox $box
    }
    $radios add $leftRadios
    $radios add $rightRadios
    $controls add $radios
    $main setElt centre $controls

    set buttons [new ::gui::HorizontalContainer 0.01 right]
    set okb [new ::gui::Button [_ A editor create] "$this create"]
    $okb setDefault
    $buttons add $okb
    set canb [new ::gui::Button [_ A gui cancel] "$this cancel"]
    $canb setCancel
    $buttons add $canb
    $main setElt bottom $buttons

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main] 0.8]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method setClass cls {
    set shipClass $cls
  }

  method setBridge brd {
    set bridgeType $brd
  }

  method cancel {} {
    $editor setMode [new ShipEditorInitMode $editor]
  }

  method create {} {
    set name [string trim [$nameField getText]]
    if {![string length $name]} return

    # OK, set the config up
    set basename [generateNewShipBasename $name]
    set mount ship:$basename
    set filename hangar/$basename.ship
    if {{} != $::abnet::userid} {
      file mkdir hangar/$::abnet::userid
    }
    $ create $filename $mount
    $ add $mount info STGroup
    $ adds $mount.info name $name
    $ adds $mount.info author $::abnet::username
    $ addi $mount.info version 0
    $ adds $mount.info bridge konnichiwa
    $ addf $mount.info reinforcement 1.0
    $ adds $mount.info class $shipClass
    $ add $mount.info alliance STArray
    $ appends $mount.info.alliance EUF
    $ appends $mount.info.alliance WUDR
    if {"" == $::abnet::userid} {
      $ addi $mount.info ownerid -1
    } else {
      $ addi $mount.info ownerid $::abnet::userid
    }
    $ addb $mount.info needs_uploading yes
    $ addb $mount.info sharing_enabled [$ bool conf.default_share_ships]
    $ add $mount cells STGroup
    $ add $mount.cells konnichiwa STGroup
    $ adds $mount.cells.konnichiwa type $bridgeType
    $ add $mount.cells.konnichiwa neighbours STArray
    $ appends $mount.cells.konnichiwa.neighbours {}
    $ appends $mount.cells.konnichiwa.neighbours {}
    $ appends $mount.cells.konnichiwa.neighbours {}
    $ appends $mount.cells.konnichiwa.neighbours {}
    $ sync $mount
    $ appends hangar.all_ships $basename
    $ appends hangar.[string tolower $shipClass] $basename
    refreshStandardHangars
    $ sync hangar

    $ sets edit.mountname $mount
    $editor manip reloadShip
    $editor setMode [new ShipEditorMain $editor]
  }
}
