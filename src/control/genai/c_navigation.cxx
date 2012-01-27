/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_navigation.hxx
 */

/*
 * c_navigation.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#include <algorithm>

#include "src/ship/ship.hxx"
#include "src/globals.hxx"
#include "c_navigation.hxx"

using namespace std;

static NavigationCortex::input_map ncim;

NavigationCortex::NavigationCortex(const libconfig::Setting& species, Ship* s,
                                   cortex_input::SelfSource* ss)
: Cortex(species, "navigation", cip_last, numOutputs, ncim),
  ship(s), score(0)
{
  setSelfSource(ss);
  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(throttle);
  CO(accel);
  CO(brake);
  CO(spin);
  #undef CO
}

void NavigationCortex::evaluate(float et, const GameObject* objective) {
  bindInputs(&inputs[0]);
  getObjectiveInputs(objective, &inputs[0]);
  ship->configureEngines(eval((unsigned)accel) > 0,
                         eval((unsigned)brake) > 0,
                         eval((unsigned)throttle));
  ship->spin(et*STD_ROT_RATE*max(-1.0f,min(+1.0f,eval((unsigned)spin))));

  float dx  = objective->getX()  - ship->getX(),
        dy  = objective->getY()  - ship->getY(),
        //Use absolute velocity instead of relative
        dvx = ship->getVX(),// - objective->getVX(),
        dvy = ship->getVY();// - objective->getVY();

  float distsq = max(1.0e-12f, dx*dx + dy*dy);
  score += et
         * (dx*dvx + dy*dvy)/distsq;
}
