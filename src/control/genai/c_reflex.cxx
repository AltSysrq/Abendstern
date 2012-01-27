/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_reflex.hxx
 */

/*
 * c_reflex.cxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#include "c_reflex.hxx"
#include "src/sim/game_field.hxx"
#include "src/ship/ship.hxx"

static ReflexCortex::input_map rim;

ReflexCortex::ReflexCortex(const libconfig::Setting& species,
                           Ship* s, cortex_input::SelfSource* ss,
                           cortex_input::EmotionSource* es)
: Cortex(species, "reflex", cip_last, numOutputs, rim),
  score(0), timeAlive(0)
{
  setField(s->getField());
  setEmotionSource(es);
  setSelfSource(ss);

  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(edge);
  CO(dodge);
  CO(runaway);
  #undef CO
}

ReflexCortex::Directive ReflexCortex::evaluate(float et) {
  bindInputs(&inputs[0]);

  Directive directive = Frontal;
  float max = 0.0f;
  float e;

  e = eval((unsigned)edge);
  if (e > max) {
    max = e;
    directive = AvoidEdge;
  }

  e = eval((unsigned)dodge);
  if (e > max) {
    max = e;
    directive = Dodge;
  }

  e = eval((unsigned)runaway);
  if (e > max) {
    max = e;
    directive = RunAway;
  }

  if (directive == Frontal) score += et;
  timeAlive += et;

  return directive;
}
