set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Label "Add Cells"]
$panel add [new ::gui::Label "Cell type:" left]

set prevbox none
foreach {name orient enum} [list \
  [_ A cell square] 0 square \
  [_ A cell circle] 0 circle \
  [_ A cell equilt] 0 equil \
  "[_ A cell rightt]-1" 0 right \
  "[_ A cell rightt]-2" 1 right \
  "[_ A cell rightt]-3" 2 right \
] {
  set box [new ::gui::RadioButton $name \
           "expr {\[$ str edit.new_cell_type\] == {$enum} &&
                  \[$ int edit.rightt_orient\] == {$orient}}" {} \
           $prevbox "$ sets edit.new_cell_type $enum
                     $ seti edit.rightt_orient $orient"]
  $panel add $box
  set prevbox $box
}

$ sets edit.current_mode create_cell
