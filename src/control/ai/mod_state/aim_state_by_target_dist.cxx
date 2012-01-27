/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_state/aim_state_by_target_dist.hxx
 */

/*
 * aim_state_by_target_dist.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <string>

#include "aim_state_by_target_dist.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

AIM_StateByTargetDist::AIM_StateByTargetDist(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  considerThisClose(s["consider_this_close"]),
  closeState((const char*)s["close_state"]),
  farState((const char*)s["far_state"])
{ }

void AIM_StateByTargetDist::action() {
  if (!ship->target.ref) return;

  float dx = ship->getX() - ship->target.ref->getX();
  float dy = ship->getY() - ship->target.ref->getY();
  float distSq = dx*dx + dy*dy;

  controller.setState(distSq < considerThisClose*considerThisClose? closeState.c_str() : farState.c_str());
}

static AIModuleRegistrar<AIM_StateByTargetDist> registrar("state/target_distance");
