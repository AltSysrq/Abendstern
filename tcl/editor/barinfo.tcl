set panel [new ::gui::VerticalContainer]
set mount [$ str edit.mountname]

$panel add [new ::gui::Label "--\a\[(special)[_ A editor general_header]\a\]--" centre]
fmtlbl $panel "[_ A editor ship_name]: %s" [$ str $mount.info.name]
if {[$ exists $mount.info.author]} {
  fmtlbl $panel "[_ A editor author]: %s" [$ str $mount.info.author]
}
fmtlbl $panel "[_ A editor class]: %s" [$ str $mount.info.class]
fmtlbl $panel "[_ A editor category]: %s"      [$ str edit.ship_category]

$panel add [new ::gui::Label "--\a\[(special)[_ A editor physical_header]\a\]--" centre]
fmtlbl $panel "[_ A editor cells]: %d"     [$ int edit.ship_cell_count]
fmtlbl $panel "[_ A editor mass]: %.1f" [$ float edit.ship_mass]
fmtlbl $panel "[_ A editor radius]: %.1f"  [$ float edit.ship_radius]
fmtlbl $panel "[_ A editor length]: %.1f"  [$ float edit.ship_length]
fmtlbl $panel "[_ A editor width]: %.1f"   [$ float edit.ship_width]
fmtlbl $panel "[_ A editor reinforcement]: %.1f" [$ float edit.ship_reinforcement]
#fmtlbl $panel "[_ A editor cost]: $%d"     [$ int edit.ship_cost]

$panel add [new ::gui::Label "--\a\[(special)[_ A editor capabilities_header]\a\]--" centre]
fmtlbl $panel "[_ A editor power_gen]: %d"       [$ int edit.ship_power_gen]
fmtlbl $panel "[_ A editor min_power_use]: %d"   [$ int edit.ship_power_use_no_accel]
fmtlbl $panel "[_ A editor max_power_use]: %d"   [$ int edit.ship_power_use_all_accel]
fmtlbl $panel "[_ A editor acceleration]: %f"         [$ float edit.ship_acceleration]
fmtlbl $panel "[_ A editor rotation]: %f"      [$ float edit.ship_rotaccel]

$panel add [new ::gui::Label "--\a\[(special)[_ A editor stealth_header]\a\]--" centre]
fmtlbl $panel "[_ A editor supported]: %s" \
  [expr {[$ bool edit.ship_supports_stealth_mode]? [_ A gui yes] : "\a\[(danger)[_ A gui no]\a\]"}]
fmtlbl $panel "[_ A editor power_gen]: %d" [$ int edit.ship_power_gen_stealth]
fmtlbl $panel "[_ A editor min_power_use]: %d" [$ int edit.ship_power_use_stealth_no_accel]
fmtlbl $panel "[_ A editor max_power_use]: %d" [$ int edit.ship_power_use_stealth_all_accel]
fmtlbl $panel "[_ A editor acceleration]: %f"         [$ float edit.ship_stealth_acceleration]
fmtlbl $panel "[_ A editor rotation]: %f"      [$ float edit.ship_stealth_rotaccel]
$ sets edit.current_mode none
