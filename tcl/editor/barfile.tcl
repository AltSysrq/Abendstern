# File menu
set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Button [_ A editor save_changes] "$this save"]
$panel add [new ::gui::Button [_ A editor save_as_fork] "$this saveFork"]
$panel add [new ::gui::Button [_ A editor revert] "
  $editor manip pushUndo
  $ revert \[$ str edit.mountname\]
  $ setb edit.modified no
  $editor manip reloadShip
  "]
$ sets edit.current_mode none
