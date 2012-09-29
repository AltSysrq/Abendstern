# Initial dialogue that allows the user to chose from
# the following options:
# + Create new ship
#   Proceeds to ShipEditorNewShip mode
# + Open existing ship
#   Proceeds to ShipEditorMain mode after loading
#   the selected ship
# + Delete ship
#   Proceeds to ShipEditorDeletor mode
# + Exit
#   Terminates ShipEditor application
class ShipEditorInitMode {
  inherit ::gui::Mode

  variable editor
  variable chooser
  variable selected

  constructor edr {::gui::Mode::constructor} {
    set editor $edr
    set outer [new ::gui::BorderContainer 0.0 0.01]
    set buttons {}
    lappend buttons [new ::gui::Button [_ A editor new_ship] "$this newShip"]
    set openButton [new ::gui::Button [_ A editor open_ship] "$this openShip"]
    $openButton setDefault
    lappend buttons $openButton
    lappend buttons [new ::gui::Button [_ A editor delete_ship] "$this delShip"]
    set exitButton [new ::gui::Button [_ A gui quit] "$editor die"]
    $exitButton setCancel
    lappend buttons $exitButton

    $outer setElt top [new ::gui::Label [_ A editor initmode_title] centre]
    set chooser [new ::gui::ShipChooser {} {{} A B C} {} $buttons]
    $outer setElt centre $chooser
#    set root [new ::gui::ComfyContainer [new ::gui::Frame $outer] 0.8]
    set root $outer
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method newShip {} {
    $editor setMode [new ShipEditorNewShip $editor]
  }

  method openShip {} {
    set sel [$chooser get-selected]
    if {$sel eq {}} return
    $ sets edit.mountname $sel
    $ setb edit.modified no
    if {[string length [$editor manip reloadShip]]} {
      $editor setMode [new ShipEditorError $editor \
                                          [_ A editor couldnt_load_ship]]
      return
    }
    $editor setMode [new ShipEditorMain $editor]
  }

  method delShip {} {
    set sel [$chooser get-selected]
    if {$sel eq {}} return
    $editor setMode [new ShipEditorDeletor $editor $sel]
  }
}

class ShipEditorError {
  inherit ::gui::Mode

  variable editor

  constructor {edr msg} {::gui::Mode::constructor} {
    set editor $edr

    set root [new ::gui::VerticalContainer 0.01 centre]
    $root add [new ::gui::Label $msg centre]
    set ok [new ::gui::Button [_ A gui ok] "$this close"]
    $ok setDefault
    $ok setCancel
    $root add $ok

    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method close {} {
    $editor setMode [new ShipEditorInitMode $editor]
  }
}
