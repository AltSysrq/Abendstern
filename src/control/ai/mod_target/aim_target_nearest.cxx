/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_target/aim_target_nearest.hxx
 */

/*
 * aim_target_nearest.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>

#include "aim_target_nearest.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"

AIM_TargetNearest::AIM_TargetNearest(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_TargetNearest::action() {
  float minDist=9999999;
  Ship* closest=NULL;
  radar_t::iterator begin, end;
  for (ship->radarBounds(begin,end); begin != end; ++begin) {
    Ship* s = *begin;
    if (!s->hasPower()) continue;
    Alliance al = getAlliance(ship->insignia, s->insignia);
    if (al != Enemies) continue;

    float dx = s->getX() - ship->getX();
    float dy = s->getY() - ship->getY();
    float dist = dx*dx + dy*dy;
    if (dist < minDist) {
      minDist = dist;
      closest = s;
    }
  }

  ship->target.assign(closest);
}

static AIModuleRegistrar<AIM_TargetNearest> registrar("target/nearest");
