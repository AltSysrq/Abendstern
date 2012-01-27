# View bar for ship editor

set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Button [_ A editor zoom_in] "$editor manip scrollUp 0.5 0.5"]
$panel add [new ::gui::Button [_ A editor zoom_out] "$editor manip scrollDown 0.5 0.5"]
$panel add [new ::gui::Button [_ A editor reset_view] "$editor manip resetView"]
$panel add [new ::gui::Checkbox [_ A editor show_shields]  "$ bool edit.show_shields" {} \
            "$ setb edit.show_shields yes; $editor manip reloadShip" \
            "$ setb edit.show_shields no ; $editor manip reloadShip"]
$panel add [new ::gui::Checkbox [_ A editor show_centre_lines] "$ bool edit.show_centre" {} \
            "$ setb edit.show_centre yes" \
            "$ setb edit.show_centre no"]
$ sets edit.current_mode none
