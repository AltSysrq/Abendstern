# Converse of acctinfo.tcl --- saves a section of abendstern.rc
# to the server.

if {"" != $::abnet::userid} {
  $ create remoterc.tmp remote
  $ add remote hud STGroup
  $ add remote control_scheme STString
  $ add remote ship_colour STArray
  $ add remote camera STGroup
  $ add remote default_share_ships STBool
  foreach c {hud control_scheme ship_colour camera
             default_share_ships} {
    confcpy remote.$c conf.$c
  }
  $ sync remote
  $ close remote
  ::abnet::putf abendstern.rc remoterc.tmp
  $state setCallback [_ A boot acctinfs] {
    if {$::abnet::busy} {
      return 0
    } else {
      file delete remoterc.tmp
      return 200
    }
  }
}
