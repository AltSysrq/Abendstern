/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_navigation/aim_avoid_edge.hxx
 */

/*
 * aim_avoid_edge.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <cmath>

#include "aim_avoid_edge.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/game_field.hxx"
#include "src/globals.hxx"

using namespace std;

AIM_AvoidEdge::AIM_AvoidEdge(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_AvoidEdge::action() {
  float cx = ship->getField()->width/2;
  float cy = ship->getField()->height/2;
  float targetTheta = atan2(cy - ship->getY(), cx - ship->getX());
  controller.setTargetTheta(targetTheta-controller.gglob("thrust_angle", 0.0f));

  float theta = ship->getRotation() + controller.gglob("thrust_angle", 0.0f);
  float diff = theta - targetTheta;
  while (diff < -pi) diff += 2*pi;
  while (diff > +pi) diff -= 2*pi;
  //Intentionally overload here, we're desperate
  if (fabs(diff) < pi/2) ship->configureEngines(true, true, 1.0f);
  else                   ship->configureEngines(false, true, 1.0f);
}

static AIModuleRegistrar<AIM_AvoidEdge> registrar("navigation/avoid_edge");
