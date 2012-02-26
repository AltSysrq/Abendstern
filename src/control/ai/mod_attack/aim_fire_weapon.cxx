/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_attack/aim_select_weapon.hxx
 */

/*
 * aim_fire_weapon.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <iostream>

#include "aim_fire_weapon.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"

using namespace std;

AIM_FireWeapon::AIM_FireWeapon(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  thresh(s.exists("thresh")? s["thresh"] : 0.85f)
{ }

void AIM_FireWeapon::action() {
  if (controller.gstat("no_appropriate_weapon", false)) return;
  if (controller.gstat("shot_quality", 0.0f) < thresh) return;

  expectedWeapon = controller.getCurrentWeapon();
  controller.delTimer(this, 0);
  controller.addTimer(this, 500.0f, 0);
}

void AIM_FireWeapon::timer(int) {
  if (controller.getCurrentWeapon() != expectedWeapon)
    controller.delTimer(this, 0);
  else {
    weapon_fire(ship, (Weapon)expectedWeapon);
    if (ship->getCurrentCapacitance() == 0) {
      //Out of energy to continue firing
      controller.delTimer(this, 0);
    }
  }
}

static AIModuleRegistrar<AIM_FireWeapon> registrar("attack/fire");
