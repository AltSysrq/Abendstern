/**
 * @file
 * @author Jason Lingle
 * @date 2012.11.13
 * @brief Implementation of src/control/ai/mod_target/aim_target_nearest_agro.hxx
 */

#include <libconfig.h++>
#include <algorithm>

#include "aim_target_nearest_agro.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"

using namespace std;

AIM_TargetNearestAgro::AIM_TargetNearestAgro(AIControl* c,
                                             const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_TargetNearestAgro::action() {
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
    float agro = max(1.0f, (float)s->playerScore);
    dist /= agro*agro;
    if (dist < minDist) {
      minDist = dist;
      closest = s;
    }
  }

  ship->target.assign(closest);
}

static AIModuleRegistrar<AIM_TargetNearestAgro>
registrar("target/nearest-agro");
