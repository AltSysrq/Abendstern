/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_aiming.hxx
 */

/*
 * c_aiming.cxx
 *
 *  Created on: 03.11.2011
 *      Author: jason
 */

#include <algorithm>

#include "c_aiming.hxx"
#include "src/ship/ship.hxx"

using namespace std;

static AimingCortex::input_map acim;

AimingCortex::AimingCortex(const libconfig::Setting& species, Ship* s,
                           cortex_input::SelfSource* ss,
                           cortex_input::CellTSource* cs)
: Cortex(species, "aiming", cip_last, numOutputs, acim),
  ship(s), score(0), strategicWeapon(-1)
{
  setSelfSource(ss);
  setCellTSource(cs);

  #define CON(out,n) compileOutput(#out #n, out+n)
  #define CO(out) CON(out,0); CON(out,1); CON(out,2); CON(out,3); \
                  CON(out,4); CON(out,5); CON(out,6); CON(out,7)
  CO(throttle);
  CO(accel);
  CO(brake);
  CO(spin);
  #undef CO
  #undef CON
}

void AimingCortex::evaluate(float et, signed w, const GameObject* obj) {
  strategicWeapon = w;
  bindInputs(&inputs[0]);
  getObjectiveInputs(obj,&inputs[0]);

  ship->configureEngines(eval(accel+w) > 0,
                         eval(brake+w) > 0,
                         eval(throttle+w));
  ship->spin(et*STD_ROT_RATE*max(-1.0f,min(+1.0f,eval(spin+w))));
}

void AimingCortex::weaponFired(signed w) {
  if (strategicWeapon == w)
    score += 1;
}
