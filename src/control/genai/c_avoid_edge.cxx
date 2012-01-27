/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_avoid_edge.hxx
 */

/*
 * c_avoid_edge.cxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#include <algorithm>
#include <cmath>

#include "src/sim/game_field.hxx"
#include "src/ship/ship.hxx"
#include "c_avoid_edge.hxx"

using namespace std;

static AvoidEdgeCortex::input_map aecim;

AvoidEdgeCortex::AvoidEdgeCortex(const libconfig::Setting& species,
                                 Ship* s, cortex_input::SelfSource* ss)
: Cortex(species, "avoid_edge", cip_last, numOutputs, aecim),
  ship(s), score(0)
{
  setSelfSource(ss);
  setField(s->getField());
  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(throttle);
  CO(accel);
  CO(brake);
  CO(spin);
  #undef CO
}

void AvoidEdgeCortex::evaluate(float et) {
  bindInputs(&inputs[0]);
  ship->configureEngines(eval((unsigned)accel) > 0,
                         eval((unsigned)brake) > 0,
                         eval((unsigned)throttle));
  ship->spin(et*STD_ROT_RATE*max(-1.0f,min(+1.0f, eval((unsigned)spin))));

  float cx = ship->getField()->width/2, cy = ship->getField()->height/2;
  float dx = cx - ship->getX(), dy = cy - ship->getY();
  //Normalise vector to centre
  float dist = sqrt(dx*dx + dy*dy);
  dx /= dist;
  dy /= dist;

  //Update score
  score += et * (dx*ship->getVX() + dy*ship->getVY());
}
