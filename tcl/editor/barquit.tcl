# Asks the user for confirmation to quit without saving

set panel [new ::gui::VerticalContainer 0.01]
set buttons [new ::gui::HorizontalContainer 0.01 centre]
$panel add [new ::gui::Label "\a\[(danger)[_ A gui warning]\a\]" centre]
$panel add [new ::gui::Label [_ A editor quit_without_saving_p] left]
$buttons add [new ::gui::Button [_ A gui yes] "$this quitReally"]
$buttons add [new ::gui::Button [_ A gui no] "$this setbar file"]
$panel add $buttons
