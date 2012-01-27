/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_weapon.hxx
 */

/*
 * c_weapon.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#include <vector>

#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/sys/launcher.hxx"
#include "c_weapon.hxx"

using namespace std;

static WeaponCortex::input_map wcim;

WeaponCortex::WeaponCortex(const libconfig::Setting& species, const char* name,
                           Ship* s, cortex_input::SelfSource* ss,
                           cortex_input::CellTSource* cs, bool noFire)
: Cortex(species, name, cip_last, numOutputs, wcim),
  enableNoFire(noFire), ship(s)
{
  setSelfSource(ss);
  setCellTSource(cs);
  #define CO(i) compileOutput("score" #i, i)
  CO(0); CO(1); CO(2); CO(3);
  CO(4); CO(5); CO(6); CO(7);
  #undef CO
}

signed WeaponCortex::evaluate(const GameObject* obj) {
  bindInputs(&inputs[0]);
  getObjectiveInputs(obj, &inputs[0]);

  signed best = -1;
  float bestScore=0;
  vector<ShipSystem*> sys;
  for (unsigned w = 0; w < numOutputs; ++w) {
    float score = 0;
    sys.clear();
    //Must explicitly qualify ::Weapon, or the compiler will
    //think we're referring to our base class, cortex_input::Weapon<...>.
    weapon_enumerate(ship, (::Weapon)w, sys);
    //Skip any evaluation (including possible replacement) if there is
    //nothing here
    if (sys.empty()) continue;

    for (unsigned i=0; i<sys.size(); ++i) {
      getWeaponInputs((Launcher*)sys[i], sys.size(), &inputs[0]);
      score += eval(w);
    }

    if ((!enableNoFire || score > 0) && (best == -1 || score > bestScore)) {
      best = w;
      bestScore = score;
    }
  }

  return best;
}
