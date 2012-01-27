set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Label "     [_ A editor ship_properties_header]     "]

set wasModified [$ bool edit.modified]

set mount [$ str edit.mountname]
if {![$ exists $mount.info.author]} {
  $ adds $mount.info author Anonymous
}
$panel add [new ::gui::TextField [_ A editor ship_name] \
              [$ str $mount.info.name] {} {} {} "$this setName"]
$panel add [new ::gui::Checkbox [_ A editor sharing_enabled] \
              "$ bool $mount.info.sharing_enabled" {} \
              "$this enableSharing" "$this disableSharing"]
$panel add [new ::gui::Slider [_ A editor reinforcement_abbr] float \
            "$ float $mount.info.reinforcement" {} 0.0 32.0 \
            0.1 "$this setReinforcement" "format {%.1f}" [$::gui::font width 99.9]]
set prevbox none
foreach class {A B C} {
  set box [new ::gui::RadioButton "[_ A editor class_prefix]\a&$class" \
           "expr {\[$ str $mount.info.class\] == {$class}}" {} \
           $prevbox "$this changeClass $class"]
  $panel add $box
  set prevbox $box
}

$ sets edit.current_mode none
$ setb edit.modified $wasModified
