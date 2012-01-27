# Ask the user whether they are sure they want to delete the ship

class ShipEditorDeletor {
  inherit ::gui::Mode

  variable editor
  variable basename
  variable index

  constructor {edr bn ix} {
    ::gui::Mode::constructor
  } {
    set editor $edr
    set basename $bn
    set index $ix

    set root [new ::gui::VerticalContainer 0.01 centre]
    set prettyname [$ str ship:$basename.info.name]
    $root add [new ::gui::Label [_ A editor delmode_title] centre]
    $root add [new ::gui::Label [_ A editor delmode_prompt] centre]
    $root add [new ::gui::Label "$prettyname?" centre]
    set buttons [new ::gui::HorizontalContainer 0.02 centre]
    set okb [new ::gui::Button [_ A gui yes] "$this accept"]
    $okb setDefault
    $buttons add $okb
    set canb [new ::gui::Button [_ A gui no] "$this abort"]
    $canb setCancel
    $buttons add $canb
    $root add $buttons

    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method accept {} {
    set mount ship:$basename
    set cls [string tolower [$ str $mount.info.class]]
    set name $basename
    $ close $mount
    file delete [shipMount2Path $mount]
    for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
      if {[$ str hangar.all_ships.\[$i\]] == $name} {
        $ remix hangar.all_ships $i
      }
    }
    for {set i 0} {$i < [$ length hangar.$cls]} {incr i} {
      if {[$ str hangar.$cls.\[$i\]] == $name} {
        $ remix hangar.$cls $i
      }
    }

    refreshStandardHangars
    $ sync hangar

    set ::shipEditorSelected [expr {$index != 0? $index-1 : 0}]

    abort
  }

  method abort {} {
    $editor setMode [new ShipEditorInitMode $editor]
  }
}
