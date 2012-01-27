/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_weapon_level.hxx
 */

/*
 * c_weapon_level.cxx
 *
 *  Created on: 03.11.2011
 *      Author: jason
 */

#include <algorithm>
#include <vector>

#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "c_weapon_level.hxx"

using namespace std;

static WeaponLevelCortex::input_map wlcim;

WeaponLevelCortex::WeaponLevelCortex(const libconfig::Setting& species, Ship* s,
                                     cortex_input::SelfSource* ss,
                                     cortex_input::CellTSource* cs)
: Cortex(species, "weapon_level", cip_last, numOutputs, wlcim),
  ship(s), score(0)
{
  setSelfSource(ss);
  setCellTSource(cs);
  #define CO(i) compileOutput("level" #i, i)
  CO(0); CO(1); CO(2); CO(3);
  CO(4); CO(5); CO(6); CO(7);
  #undef CO
}

signed WeaponLevelCortex::evaluate(signed w, const GameObject* obj) {
  bindInputs(&inputs[0]);
  getObjectiveInputs(obj,&inputs[0]);

  vector<ShipSystem*> weapons;
  weapon_enumerate(ship, (::Weapon)w, weapons);

  if (weapons.empty()) return 0; //Singularity

  float sum=0;

  for (unsigned i=0; i<weapons.size(); ++i) {
    getWeaponInputs((const Launcher*)weapons[i], weapons.size(), &inputs[0]);
    sum += eval(w);
  }

  return (signed)max(0.0f, min(1000.0f, sum/weapons.size()));
}

void WeaponLevelCortex::damageDealt(float amt) {
  score += amt;
}

void WeaponLevelCortex::firingFailed() {
  score /= 2;
}
