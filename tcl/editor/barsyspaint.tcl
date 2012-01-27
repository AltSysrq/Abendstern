# Root system painting interface pane
# Show the current settings and allows
# to change them

set panel [new ::gui::VerticalContainer 0.02]
set sys0i [new ::gui::VerticalContainer 0.01]
set sys1i [new ::gui::VerticalContainer 0.01]
set modes [new ::gui::VerticalContainer 0.01]

set dosys {
  switch -exact [$ str edit.sys${s}_type] {
    delete                      {$p add [new ::gui::Label "\a\[(danger)[_ A editor delete]\a\]" left]}
    none                        {$p add [new ::gui::Label [_ A editor no_change] left]}
    ShieldGenerator {
      $p add [new ::gui::Label [_ A sys shield_generator] left]
      $p add [new ::gui::Slider [_ A editor shield_radius] \
              float "$ float edit.sys${s}_shield_radius" {} 2.0 15.0 \
              0.1 "$ setf edit.sys${s}_shield_radius" {format "%4.1f"} [$::gui::font width 99.9]]
      $p add [new ::gui::Slider [_ A editor shield_strength] \
              float "$ float edit.sys${s}_shield_strength" {} 5.0 50.0 \
              1.0 "$ setf edit.sys${s}_shield_strength" {format "%4.1f"} [$::gui::font width 99.9]]
    }
    Capacitor {
      $p add [new ::gui::Label [_ A sys capacitor] left]
      # We name the slider Amt so that we don't change the hotkeys for the
      # two Change buttons
      $p add [new ::gui::Slider [_ A editor capac_amt] int "$ int edit.sys${s}_capacitance" {} 10 100 \
              5 "$ seti edit.sys${s}_capacitance" {format "%i"} [$::gui::font width 100]]
    }
    GatlingPlasmaBurstLauncher  {addLabelsWithWordBreaks $p [_ A sys gatling_plasma_burst_launcher]
      $p add [new ::gui::Checkbox [_ A editor gpbl_turbo] "$ bool edit.sys${s}_gatling_turbo" {} \
              "$ setb edit.sys${s}_gatling_turbo yes" \
              "$ setb edit.sys${s}_gatling_turbo no"]
    }
    DispersionShield            {addLabelsWithWordBreaks $p [_ A sys dispersion_shield]}
    FissionPower                {addLabelsWithWordBreaks $p [_ A sys fission_power]}
    FusionPower                 {addLabelsWithWordBreaks $p [_ A sys fusion_power]}
    PowerCell                   {addLabelsWithWordBreaks $p [_ A sys power_cell]}
    AntimatterPower             {addLabelsWithWordBreaks $p [_ A sys antimatter_power]}
    Heatsink                    {addLabelsWithWordBreaks $p [_ A sys heatsink]}
    ParticleAccelerator         {addLabelsWithWordBreaks $p [_ A sys particle_accelerator]}
    SuperParticleAccelerator    {addLabelsWithWordBreaks $p [_ A sys super_particle_accelerator]}
    BussardRamjet               {addLabelsWithWordBreaks $p [_ A sys bussard_ramjet]}
    MiniGravwaveDrive           {addLabelsWithWordBreaks $p [_ A sys mini_gravwave_drive]}
    MiniGravwaveDriveMKII       {addLabelsWithWordBreaks $p [_ A sys mini_gravwave_drive_ii]}
    RelIonAccelerator           {addLabelsWithWordBreaks $p [_ A sys rel_ion_accelerator]}
    EnergyChargeLauncher        {addLabelsWithWordBreaks $p [_ A sys energy_charge_launcher]}
    MagnetoBombLauncher         {addLabelsWithWordBreaks $p [_ A sys magneto_bomb_launcher]}
    PlasmaBurstLauncher         {addLabelsWithWordBreaks $p [_ A sys plasma_burst_launcher]}
    SemiguidedBombLauncher      {addLabelsWithWordBreaks $p [_ A sys semiguided_bomb_launcher]}
    MonophasicEnergyEmitter     {addLabelsWithWordBreaks $p [_ A sys monophasic_energy_emitter]}
    MissileLauncher             {addLabelsWithWordBreaks $p [_ A sys missile_launcher]}
    ParticleBeamLauncher        {addLabelsWithWordBreaks $p [_ A sys particle_beam_launcher]}
    ReinforcementBulkhead       {addLabelsWithWordBreaks $p [_ A sys reinforcement_bulkhead]}
    SelfDestructCharge          {addLabelsWithWordBreaks $p [_ A sys self_destruct_charge]}
    CloakingDevice              {addLabelsWithWordBreaks $p [_ A sys cloaking_device]}
  }

  $p add [new ::gui::Button [_ A editor change_dots] "set ::systemToEdit $s; $this setbar sysedit"]
}

$sys0i add [new ::gui::Label [_ A editor primary_header] centre]
set p $sys0i
set s 0
eval $dosys
$panel add $sys0i

$sys1i add [new ::gui::Label [_ A editor secondary_header] centre]
set p $sys1i
set s 1
eval $dosys
$panel add $sys1i

$ sets edit.current_mode add_system_brush

$modes add [new ::gui::Label [_ A editor paint_mode] centre]
set paintbox [new ::gui::RadioButton [_ A editor paint_mode_brush] {
  expr {[$ str edit.current_mode] == "add_system_brush"}
} {} none {$ sets edit.current_mode add_system_brush}]
$modes add $paintbox
$modes add [new ::gui::RadioButton [_ A editor paint_mode_rect] {
  expr {[$ str edit.current_mode] == "add_system_rect"}
} {} $paintbox {$ sets edit.current_mode add_system_rect}]
$panel add $modes

$panel add [new ::gui::Button [_ A editor copy_from_cell] "$this setbar syscopy"]
