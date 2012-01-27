/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_introspection/aim_intro_weapon.hxx
 */

/*
 * aim_intro_weapon.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <cstdlib>
#include <libconfig.h++>

#include "aim_intro_weapon.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/sys/launcher.hxx"
#include "src/globals.hxx"

using namespace std;

AIM_IntroWeapon::AIM_IntroWeapon(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_IntroWeapon::action() {
  int weaponIx = controller.gglob("weapon_introspection_count", 0);
  if (weaponIx < 0) weaponIx = 0;
  controller.sglob("weapon_introspection_count", weaponIx+1);

  Weapon weap;
  if (weaponIx < (int)controller.numWeapons)
    weap = (Weapon)weaponIx;
  else
    weap = (Weapon)(rand() % controller.numWeapons);

  AIControl::WeaponInfo& info(controller.getWeaponInfo((int)weap));
  info.clear();
  //Loop through cells and systems for launchers of this type
  for (unsigned i=0; i<ship->cells.size(); ++i) for (unsigned s=0; s<2; ++s) {
    if (ship->cells[i]->systems[s]
    &&  ship->cells[i]->systems[s]->clazz == Classification_Weapon
    &&  ship->cells[i]->systems[s]->weaponClass == weap) {
      AIControl::SingleWeaponInfo swi;
      swi.theta = ((Launcher*)ship->cells[i]->systems[s])->getLaunchAngle();
      while (swi.theta < 0) swi.theta += 2*pi;
      while (swi.theta >= 2*pi) swi.theta -= 2*pi;
      swi.relx = ship->cells[i]->getX();
      swi.rely = ship->cells[i]->getY();
      info.push_back(swi);
    }
  }

  //Check for weapons existence
  bool hasWeapons=false;
  for (unsigned i=0; i<controller.numWeapons && !hasWeapons; ++i)
    hasWeapons = !controller.getWeaponInfo((int)i).empty();

  controller.sglob("ship_has_weapons", hasWeapons);
}

static AIModuleRegistrar<AIM_IntroWeapon> registrar("introspection/weapons");
