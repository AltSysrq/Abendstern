# Starts a staging environment for the GeneticAI;
# this will randomly select effective hangers from
# "AI Staging A", "AI Staging B", and "AI Staging C",
# and random teams between 1 and 6, inclusive (where
# 1 is actually teamless deathmatch).
#
# After creating the state, it disconnects its vpeer
# zero (the non-existent human). It sets an after
# script to run the aistaging boot catalogue after
# three minutes, then switches the BootManager to
# the new state.
#
# This is only usable in headless mode, and should
# be run in fast mode as well (ie, abendstern -H -F)
#
# You should make the contents of headless.tcl (in
# Abendstern's root) something like
#   ::abnet::openConnection
#   vwait ::abnet::busy
#   if {!$::abnet::isReady} exit
#   ::abnet::login username [::sha2::sha256 hunter2]
#   vwait ::abnet::busy
#   if {!$::abnet::success} exit
#   $state setCatalogue aistaging

if {$::abnet::isConnected} {

set humanShipMount ship:1/interceptor ;# Doesn't matter
set TEAMKILL_PENGLTY -1 ;# Let the AI learn to fight easier
set DEFAULT_MATCH_TIME [expr {999999999}] ;# Essentially disable matches
#set AUTOBOT_AI_STD_PROB 0 ;# Prevent the "spin in circles and shoot" strategy
set aistage_possibleHangars {
  {AI Staging A}
  {AI Staging B}
  {AI Staging C}
}

set h [expr {int(rand()*[llength $aistage_possibleHangars])}]
set h [lindex $aistage_possibleHangars $h]
# Search for the hangar of that name and make it effective
for {set i 0} {$i < [$ length hangar.user]} {incr i} {
  if {$h == [$ str hangar.user.\[$i\].name]} {
    makeHangarEffective $i
    break
  }
}

set t [expr {int(rand()*5+1)}]
if {$t == 1} {
  set aistage_state [new G_DM [expr {rand()*16+24}] [expr {rand()*16+24}] \
                              {new StarField default 0 $field 1} \
                              [expr {int(rand()*16+24)}]]
} else {
  set aistage_state [new G_XTDM [expr {rand()*16+24}] [expr {rand()*16+24}] \
                                {new StarField default 0 $field 1} \
                                [expr {int(rand()*16+24)}] $t]
}
$aistage_state disconnectVPeer 0
after 180000 "$aistage_state finish \[new BootManager aistaging\]"
$state setReturn $aistage_state

} else {
  # Need to reconnect
  log "Connection failed: $::abnet::resultMessage, trying again"
  after 300000 ;# Wait five minutes
  source headless.tcl
}
