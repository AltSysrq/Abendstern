/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_dodge.hxx
 */

/*
 * c_dodge.cxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#include <algorithm>

#include "src/ship/ship.hxx"
#include "c_dodge.hxx"

using namespace std;

static DodgeCortex::input_map dcim;

DodgeCortex::DodgeCortex(const libconfig::Setting& species,
                         Ship* s, cortex_input::SelfSource* ss,
                         cortex_input::EmotionSource* es)
: Cortex(species, "dodge", cip_last, numOutputs, dcim),
  ship(s),
  score(0), timeAlive(0)
{
  setSelfSource(ss);
  setEmotionSource(es);
  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(throttle);
  CO(accel);
  CO(brake);
  CO(spin);
  #undef CO
}

void DodgeCortex::evaluate(float et) {
  timeAlive += et;
  bindInputs(&inputs[0]);
  ship->configureEngines(eval((unsigned)accel) > 0,
                         eval((unsigned)brake) > 0,
                         eval((unsigned)throttle));
  ship->spin(et*STD_ROT_RATE*max(-1.0f,min(+1.0f,eval((unsigned)spin))));
}

void DodgeCortex::damage(float amt) {
  score -= amt;
}
