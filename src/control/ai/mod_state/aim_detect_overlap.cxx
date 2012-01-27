/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_state/aim_detect_overlap.hxx
 */

/*
 * aim_detect_overlap.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <string>

#include "aim_detect_overlap.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

AIM_DetectOverlap::AIM_DetectOverlap(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  overlap(s.exists("overlap")? (const char*)s["overlap"] : ""),
  otherwise(s.exists("otherwise")? (const char*)s["otherwise"] : "")
{ }

void AIM_DetectOverlap::action() {
  GameObject* t=ship->target.ref;
  if (!t) {
    controller.setState(otherwise.c_str());
    return;
  }

  float overdist = t->getRadius() + ship->getRadius();
  float overdistSq = overdist*overdist;

  float dx = t->getX() - ship->getX();
  float dy = t->getY() - ship->getY();

  if (dx*dx + dy*dy < overdistSq)
    controller.setState(overlap.c_str());
  else
    controller.setState(otherwise.c_str());
}

static AIModuleRegistrar<AIM_DetectOverlap> registrar("state/detect_overlap");
