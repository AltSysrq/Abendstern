/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_opp_weapon.hxx
 */

/*
 * c_opp_weapon.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#include "c_opp_weapon.hxx"

OppWeaponCortex::OppWeaponCortex(const libconfig::Setting& species, Ship* s,
                                 cortex_input::SelfSource* ss,
                                 cortex_input::CellTSource* cs)
: WeaponCortex(species, "opportunistic_weapon", s, ss, cs, true),
  score(0), nshots(0)
{ }

void OppWeaponCortex::damageDealt(float amt) {
  score += amt;
}
