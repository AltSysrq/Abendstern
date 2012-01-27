/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_strat_weapon.hxx
 */

/*
 * c_strat_weapon.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#include "c_strat_weapon.hxx"

StratWeaponCortex::StratWeaponCortex(const libconfig::Setting& species, Ship* s,
                                     cortex_input::SelfSource* ss,
                                     cortex_input::CellTSource* cs)
: WeaponCortex(species, "strategic_weapon", s, ss, cs, false),
  score(0), requestedWeapon(-1)
{ }

signed StratWeaponCortex::evaluate(const GameObject* go) {
  return requestedWeapon = WeaponCortex::evaluate(go);
}

void StratWeaponCortex::weaponFired(signed which) {
  score += (which == requestedWeapon? +4 : -1);
}
