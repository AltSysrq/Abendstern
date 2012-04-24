# Performs any hotpatching necessary for backwards-compatibility

# If the hangar doesn't exist, copy hangar.default to hangar
if {![file exists hangar]} {
  file copy hangar.default hangar
}

# Ensure that the patchlevel.rc file exists
if {![file exists patchlevel.rc]} {
  set f [open patchlevel.rc w]
  puts $f {}
  close $f
}

$ open patchlevel.rc patch-info

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

# Work around bug in updater relating to DLLs in subdirectories
# (This is required for smooth upgrading from pre-2011.05.05 versions)
set dontLoadAbnet no
#if {[file exists tls1.6/tls16.lld] && ![file exists tls1.6/tls16.dll]} {
#  catch {file copy tls1.6/tls16.lld tls1.6/tls16.dll}
#  set dontLoadAbnet yes
#}

if {$dontLoadAbnet} {
  # Show a message to the user indicating they must restart Abendstern
  # for ten seconds, then exit
  set restartMessageExit [expr {[clock seconds]+10}]
  $state setCallback "Abendstern must be restarted" {
    if {[clock seconds] >= $::restartMessageExit} {
      exit
    } else {
      return 0
    }
  }
}

# Migrate from all ships under hangar/ to hangar/userid
# with userid = "local" for local users.
# Any ships by me (most of which shipped with the distribution)
# are automatically moved into hangar/0, and anything else
# to hangar/local
set ALTSYSRQ_SHIPS {
  a-1701.ship
  accubierre.ship
  albatross.ship
  boson.ship
  cicaida.ship
  cloaked.ship
  colonial.ship
  cylon.ship
  dart.ship
  defiant.ship
  drill.ship
  eagle.ship
  enemy.ship
  enterprise.ship
  falcon.ship
  feuereng.ship
  geistere.ship
  grasshop.ship
  haifisc0.ship
  haifisch2.ship
  haifisch3.ship
  haifisch4.ship
  haifisch5.ship
  haifisch.ship
  hawk.ship
  interceptor.ship
  ju88.ship
  junker.ship
  klingon.ship
  komet.ship
  kosmiche.ship
  libelle.ship
  mantis.ship
  meteor.ship
  microstealth.ship
  minimumb.ship
  muon.ship
  neu_panzer.ship
  neutrino.ship
  odyssey.ship
  panzer.ship
  photon.ship
  player0.ship
  protocannon.ship
  puddleju.ship
  railgun.ship
  reiseeng.ship
  rocket.ship
  schatten.ship
  schleich.ship
  schwalbe2.ship
  schwalbe.ship
  schwertw.ship
  sclachte.ship
  scorpion.ship
  seeletoeter.ship
  sidewinder.ship
  silberfi.ship
  sonderba.ship
  spider.ship
  spindle.ship
  tbone.ship
  tie_b.ship
  tie_f.ship
  todengel.ship
  toserpise.ship
  vaufisch.ship
  vau.ship
  viking.ship
  vogelfresser.ship
  x01.ship
  x02.ship
  x03.ship
  x04.ship
  xwing.ship
  y01.ship
}

if {![file exists hangar/local]} {
  file mkdir hangar/local hangar/1
  foreach ship [glob -nocomplain -directory hangar -tails *.ship] {
    if {-1 == [lsearch -exact $ALTSYSRQ_SHIPS $ship]} {
      file rename hangar/$ship hangar/local/$ship
      set hotpatchShipUserdir([shipPath2Name $ship]) local
    } else {
      file rename hangar/$ship hangar/1/$ship
      set hotpatchShipUserdir([shipPath2Name $ship]) 1
    }
  }

  # Update user hangars to point to the new ships
  $ open hangar/list.rc hangar
  for {set i 0} {$i < [$ length hangar.user]} {incr i} {
    set p hangar.user.\[$i\].contents
    for {set j 0} {$j < [$ length $p]} {incr j} {
      set t [$ str $p.\[$j\].target]
      if {[info exists hotpatchShipUserdir($t)]} {
        $ sets $p.\[$j\].target "$hotpatchShipUserdir($t)/$t"
      }
    }
  }
  $ close hangar
}

# Add missing conf.game.use_geneticai
# (The rest of the code is OK with it missing, but this
#  exposes its existence.)
if {![$ exists conf.game.use_geneticai]} {
  $ addb conf.game use_geneticai no
}
