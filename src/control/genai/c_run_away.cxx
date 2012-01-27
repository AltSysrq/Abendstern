/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_run_away.hxx
 */

/*
 * c_run_away.cxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#include <algorithm>

#include "src/ship/ship.hxx"
#include "c_run_away.hxx"

using namespace std;

static RunAwayCortex::input_map rucip;

RunAwayCortex::RunAwayCortex(const libconfig::Setting& species,
                             Ship* s,
                             cortex_input::SelfSource* ss,
                             cortex_input::EmotionSource* es)
: Cortex(species, "run_away", cip_last, numOutputs, rucip),
  ship(s),
  score(0), timeAlive(0), usedLastFrame(false)
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

void RunAwayCortex::evaluate(float et) {
  bindInputs(&inputs[0]);

  if (usedLastFrame) {
    score += max(lastFrameNervous - inputs[cip_nervous],0.0f);
    timeAlive += et;
  }
  usedLastFrame = true;
  lastFrameNervous = inputs[cip_nervous];

  ship->configureEngines(eval((unsigned)accel) > 0,
                         eval((unsigned)brake) > 0,
                         eval((unsigned)throttle));
  ship->spin(et*STD_ROT_RATE*max(-1.0f,min(+1.0f,eval((unsigned)spin))));
}
