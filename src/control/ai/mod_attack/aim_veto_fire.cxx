/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Implementation of the AIM_VetoFire AI module
 */

#include <cmath>
#include <cstdlib>
#include <list>

#include <libconfig.h++>

#include "aim_veto_fire.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/blast.hxx"

using namespace libconfig;
using namespace std;

AIM_VetoFire::AIM_VetoFire(AIControl* c, const Setting& s)
: AIModule(c),
  trials(s.exists("trials")? s["trials"] : 16)
{
}

static bool compareObjects(const GameObject* a, const GameObject* b) {
  return a->getX() < b->getX();
}

void AIM_VetoFire::action() {
  if (!ship->target.ref) return;
  if (controller.gstat("no_appropriate_weapon", false)) return;

  float dx = ship->target.ref->getX() - ship->getX();
  float dy = ship->target.ref->getY() - ship->getY();
  float maxdist = sqrt(dx*dx + dy*dy);

  float angle = atan2(dy,dx) + controller.gstat("suggest_angle_offset", 0);
  float startx = ship->getX();
  float starty = ship->getY();
  float c = cos(angle), s = sin(angle);
  float radius = ship->getRadius();

  //Get a list of ships within the maximum distance, including dead ones
  list<Ship*> potential;

  {
    GameField::iterator it = ship->getField()->begin(),
                      end = ship->getField()->end();
    //Create dummy blast object
    Blast dummy(ship->getField(), 0, ship->getX()-maxdist,
                0, 0, 0, false, 0, false);
    it = lower_bound(it, end, &dummy, compareObjects);

    //Find ships that aren't us and put them into the list
    while (it != end) {
      if ((*it)->getClassification() == GameObject::ClassShip
      &&  (*it) != ship) {
        float xo = ship->getX() - (*it)->getX();
        float yo = ship->getY() - (*it)->getY();
        float distsq = xo*xo + yo*yo;
        float mdist = maxdist + (*it)->getRadius();
        if (distsq < mdist*mdist) {
          //It's a ship that isn't us, and it's close enough
          potential.push_back((Ship*)(*it));
        }
      }
      ++it;
    }
  }

  //Perform trials
  for (unsigned t = 0; t < trials; ++t) {
    float portion = rand()/(float)RAND_MAX;
    float px = startx + c*portion*maxdist;
    float py = starty + s*portion*maxdist;

    //Scan objects for collisions
    for (list<Ship*>::const_iterator it = potential.begin();
         it != potential.end(); ++it) {
      float xo = px - (*it)->getX();
      float yo = py - (*it)->getY();
      float distsq = xo*xo + yo*yo;
      float rad = (*it)->getRadius();
      if (distsq < rad*rad) {
        //Collision.
        //If within our own radius (portion*maxdist < ship->getRadius())
        //or if this is a friend, veto
        if (portion*maxdist < radius
        ||  (Allies == getAlliance(ship->insignia, (*it)->insignia)
             && (*it)->hasPower())) {
          controller.sstat("veto_fire", true);
          return;
        }
      }
    }
  }

  //No objection
  controller.sstat("veto_fire", false);
}

static AIModuleRegistrar<AIM_VetoFire> registrar("attack/veto_fire");
