/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_objective.hxx
 */

/*
 * ci_objective.cxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#include <cmath>

#include "src/sim/game_object.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"
#include "ci_objective.hxx"

using namespace std;

void cortex_input::objectiveGetInputs(const GameObject* go, float* dst) {
  dst[ObjectiveInputOffsets::ox] = go->getX();
  dst[ObjectiveInputOffsets::oy] = go->getY();
  dst[ObjectiveInputOffsets::ot] = go->getRotation();
  dst[ObjectiveInputOffsets::ovx] = go->getVY();
  dst[ObjectiveInputOffsets::ovy] = go->getVY();
  dst[ObjectiveInputOffsets::orad] = go->getRadius();
  if (go->getClassification() == GameObject::ClassShip) {
    const Ship* s = (const Ship*)go;
    dst[ObjectiveInputOffsets::ovt] = s->getVRotation();
    dst[ObjectiveInputOffsets::omass] = s->getMass();
  } else {
    dst[ObjectiveInputOffsets::ovt] = 0;
    dst[ObjectiveInputOffsets::omass] = 0;
  }
  //Normalise theta
  float theta = dst[ObjectiveInputOffsets::ot];
  theta = fmod(theta, 2*pi);
  if (theta > pi) theta -= 2*pi;
  else if (theta <= -pi) theta += 2*pi;
  dst[ObjectiveInputOffsets::ot] = theta;
}
