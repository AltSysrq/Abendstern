# Performs any hotpatching necessary for backwards-compatibility

# Make sure the Abendstern home directory exists
if {![file exists [homeq {}]]} {
  file mkdir [homeq {}]
  # abendstern.rc will be saved under there automatically
}

# Move hangar if needed
if {[file exists hangar]} {
  file copy hangar [homeq hangar]
  file delete -force hangar
}

# If the hangar doesn't exist, copy hangar.default to hangar
if {![file exists [homeq hangar]]} {
  file copy hangar.default [homeq hangar]
}

# Make sure than any analogue controls set to "none"
# are configured to recentre
# This is necessary due to a change in the behaviour
# of displaying/hiding the cursor on 2011.03.09
set player1_control [$ str conf.control_scheme]
if {"none" == [$ str conf.$player1_control.analogue.horiz.action]
&&  ![$ bool conf.$player1_control.analogue.horiz.recentre]} {
  $ setb conf.$player1_control.analogue.horiz.recentre yes
}
if {"none" == [$ str conf.$player1_control.analogue.vert.action]
&&  ![$ bool conf.$player1_control.analogue.vert.recentre]} {
  $ setb conf.$player1_control.analogue.vert.recentre yes
}

# Add FPS setting if not present
if {![$ exists conf.hud.show_fps]} {
  $ addb conf.hud show_fps 0
}

# Add missing conf.game.use_geneticai
# (The rest of the code is OK with it missing, but this
#  exposes its existence.)
if {![$ exists conf.game.use_geneticai]} {
  $ addb conf.game use_geneticai no
}

# Create custom control set if none exists
if {![$ exists conf.custom_control]} {
  $ add conf custom_control STGroup
  confcpy conf.custom_control conf.default_control
  $ sets conf.custom_control.comment "*custom"
}
