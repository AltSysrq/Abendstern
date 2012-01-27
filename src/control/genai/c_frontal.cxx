/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_frontal.hxx
 */

/*
 * c_frontal.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */
#include <map>
#include <utility>
#include <cassert>
#include <cstdlib>

#include "src/sim/game_field.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"
#include "c_frontal.hxx"

using namespace std;

/*
 * Global map of insignia,GameObject pairs to FrontalCortices.
 * Insignias are that of the owned cortex, not the object in the pair
 * (only cortices of the same insignia can communicate).
 * Objects here are NEVER dereferenced, so dangling pointers are OK.
 */
typedef multimap<pair<unsigned long,GameObject*>,FrontalCortex*> fcmap_t;
static fcmap_t fcmap;

static FrontalCortex::input_map fcim;

FrontalCortex::FrontalCortex(const libconfig::Setting& species, Ship* s,
                             cortex_input::SelfSource* ss)
: Cortex(species, "frontal", cipe_last, numOutputs, fcim),
  ship(s), objective(NULL), objectiveScore(0),
  distanceParm(parm("distance")),
  scoreWeightParm(parm("score_weight")),
  dislikeWeightParm(parm("dislike_weight")),
  happyWeightParm(parm("happy_weight")),
  insertedInsignia(0), timeUntilRescan(0)
{
  setSelfSource(ss);
  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(target);
  #undef CO
}

FrontalCortex::~FrontalCortex() {
  unmap();
}

void FrontalCortex::unmap() {
  if (insertedInsignia) {
    pair<fcmap_t::iterator,fcmap_t::iterator> it =
        fcmap.equal_range(make_pair(insertedInsignia, insertedObjective));
    bool found = false;
    for (; it.first != it.second; ++it.first) {
      if (it.first->second == this) {
        found = true;
        fcmap.erase(it.first);
        break;
      }
    }
    assert(found);
    insertedInsignia = 0;
  }
}

void FrontalCortex::mapins() {
  if (insertedInsignia) unmap();
  if (!objective.ref || !ship->insignia) return;

  insertedInsignia = ship->insignia;
  insertedObjective = objective.ref;
  fcmap.insert(make_pair(make_pair(insertedInsignia,insertedObjective), this));
}

float FrontalCortex::getScore() const {
  return ship->score;
}

FrontalCortex::Directive FrontalCortex::evaluate(float et) {
  timeUntilRescan -= et;
  if (timeUntilRescan <= 0 || !objective.ref) {
    timeUntilRescan = rand()*1024+1024;

    //Search objectives
    float max;
    GameObject* obj = NULL;

    //First, refresh basic inputs
    bindInputs(&inputs[0]);

    //Ships on radar
    radar_t::iterator curr,end;
    for (ship->radarBounds(curr,end); curr != end; ++curr) {
      Ship* s = *curr;
      if (Enemies == getAlliance(ship->insignia, s->insignia)) {
        //Set object-specific inputs
        getObjectiveInputs(s,&inputs[0]);
        inputs[cip_opriority] =
            scoreWeightParm * s->score
          + dislikeWeightParm * ship->getDamageFrom(s->blame)
          + happyWeightParm * s->getDamageFrom(ship->blame);
        inputs[cip_ocurr] = (s == objective.ref);
        inputs[cip_otel] = 0;
        for (pair<fcmap_t::iterator,fcmap_t::iterator> it =
               fcmap.equal_range(make_pair(ship->insignia,s));
             it.first != it.second; ++it.first)
          if (it.first->second != this)
            inputs[cip_otel] += it.first->second->objectiveScore;

        float score = eval((unsigned)target);
        if (!obj || score > max) {
          max = score;
          obj = s;
        }
      }
    }

    //Update data
    objective.assign(obj);
    ship->target.assign(obj && obj->getClassification() == GameObject::ClassShip?
                        obj : NULL);
    objectiveScore = max;
    mapins();
  }
  //If the insignia was changed, reïnsert
  if (ship->insignia != insertedInsignia) mapins();

  Directive dir;
  dir.objective = objective.ref;
  if (dir.objective && GameObject::ClassShip == dir.objective->getClassification()) {
    float dx = dir.objective->getX() - ship->getX();
    float dy = dir.objective->getY() - ship->getY();
    dir.mode = (dx*dx + dy*dy < distanceParm*distanceParm?
                Directive::Attack : Directive::Navigate);
  } else {
    dir.mode = Directive::Navigate;
  }
  return dir;
}
