# Edit pane for ship editor

set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Button [_ A editor undo]      "$editor manip popUndo; $editor manip reloadShip"]
$panel add [new ::gui::Button [_ A editor history]   "$this setbar history"]
$panel add [new ::gui::Button [_ A editor new_cells] "$this setbar mkcell"]
$panel add [new ::gui::Button [_ A editor del_cells] "$this setbar rmcell"]
$panel add [new ::gui::Button [_ A editor rot_cells] "$this setbar cellrot"]
$panel add [new ::gui::Button [_ A editor set_sys  ] "$this setbar syspaint"]
$panel add [new ::gui::Button [_ A editor rot_sys  ] "$this setbar sysrot"]
$panel add [new ::gui::Button [_ A editor move_bridge] "$this setbar movebridge"]
$ sets edit.current_mode none