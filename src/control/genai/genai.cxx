/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/genai.cxx
 */

/*
 * genai.cxx
 *
 *  Created on: 05.11.2011
 *      Author: jason
 */

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "genai.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"

using namespace std;
using namespace libconfig;

GenAI::GenAI(Ship* s, unsigned spec, const Setting& speciesRoot)
: Controller(s), SelfSource(s), EmotionSource(s),
  reflex                (speciesRoot, s, this, this),
  avoidEdge             (speciesRoot, s, this),
  dodge                 (speciesRoot, s, this, this),
  runAway               (speciesRoot, s, this, this),
  frontal               (speciesRoot, s, this),
  navigation            (speciesRoot, s, this),
  targetAnalysis        (speciesRoot, s),
  stratWeapon           (speciesRoot, s, this, this),
  aiming                (speciesRoot, s, this, this),
  oppWeapon             (speciesRoot, s, this, this),
  weaponLevel           (speciesRoot, s, this, this),
  recentTargetDamage(0),
  recentTarget(NULL),
  usedDodgeLastFrame(false),
  species(spec),
  generation(speciesRoot["generation"])
{ }

GenAI* GenAI::makeGenAI(Ship* ship) throw() {
  static char mount[256];
  static const char*const cortices[] = {
    "reflex",
    "avoid_edge",
    "dodge",
    "run_away",
    "frontal",
    "navigation",
    "target_analysis",
    "strategic_weapon",
    "aiming",
    "opportunistic_weapon",
    "weapon_level",
  };
  try {
    //Do any species exist?
    if (0 == conf["genai"]["species"].getLength()) return NULL;
    //Select one at random
    unsigned ix = rand() % conf["genai"]["species"].getLength();
    //Make sure name will fit (plus for for "ai:" prefix and term NUL)
    if (strlen(conf["genai"]["species"][ix].getName())+4 > lenof(mount)) return NULL;
    //Get mount name
    sprintf(mount, "ai:%s", conf["genai"]["species"][ix].getName());
    //Make sure top-level is valid
    conf[mount]["generation"].operator unsigned();
    for (unsigned i=0; i<lenof(cortices); ++i)
      if (Setting::TypeList != conf[mount][cortices[i]].getType())
        return NULL;
    //Cortices gracefully handle their own validation, so no need to check there
    //Configuration checks out
    try { return new GenAI(ship, ix, conf[mount]); }
    catch (...) { assert(false); } //Should never throw exception
  } catch (...) {
    //Assumptions of config failed
    return NULL;
  }
}

void GenAI::update(float et) noth {
  SelfSource::reset();
  EmotionSource::update(et);
  CellTSource::reset();

  //Check damage tracking
  if (ship->target.ref != recentTarget) {
    recentTarget = (Ship*)ship->target.ref;
    recentTargetDamage = recentTarget? recentTarget->getDamageFrom(ship->blame) : 0;
    setCellTarget(NULL);
  }

  if (recentTarget) {
    float dmg = recentTarget->getDamageFrom(ship->blame);
    if (dmg > recentTargetDamage) {
      float amt = dmg - recentTargetDamage;
      oppWeapon.damageDealt(amt);
      weaponLevel.damageDealt(amt);
    }
    recentTargetDamage = dmg;
  }

  usedDodgeLastFrame = false;

  ReflexCortex::Directive refx = reflex.evaluate(et);
  switch (refx) {
    case ReflexCortex::AvoidEdge:
      avoidEdge.evaluate(et);
      runAway.notUsed();
      return;
    case ReflexCortex::Dodge:
      dodge.evaluate(et);
      usedDodgeLastFrame = true;
      runAway.notUsed();
      return;
    case ReflexCortex::RunAway:
      runAway.evaluate(et);
      return;
    case ReflexCortex::Frontal: break;
  }
  runAway.notUsed();

  FrontalCortex::Directive obj = frontal.evaluate(et);
  if (!obj.objective) {
    //Park
    ship->configureEngines(false, true, 1);
    return;
  }

  if (obj.mode == FrontalCortex::Directive::Navigate) {
    navigation.evaluate(et, obj.objective);
  } else /* Attack */ {
    Ship* tgt = (Ship*)ship->target.ref;
    assert(tgt && tgt == obj.objective);
    Cell* ct = targetAnalysis.evaluate(et,tgt);
    if (ct) setCellTarget(ct);

    signed sw = stratWeapon.evaluate(tgt);
    if (sw == -1) {
      //No weapons available, self destruct and run away
      selfDestruct(ship);
      ship->configureEngines(true,false,1.0f);
      return;
    }
    assert(sw >= 0 && sw < 8);
    aiming.evaluate(et, sw, tgt);
    signed ow = oppWeapon.evaluate(tgt);
    if (ow != -1 && weapon_isReady(ship, (Weapon)ow)) {
      assert(ow >= 0 && ow < 8);
      signed lvl = weaponLevel.evaluate(ow,tgt);

      if (lvl > 0) {
        weapon_setEnergyLevel(ship, (Weapon)ow, lvl);
        weapon_fire(ship, (Weapon)ow);
        if (ship->getCurrentCapacitance() > 0) {
          stratWeapon.weaponFired(ow);
          oppWeapon.weaponFired();
          aiming.weaponFired(ow);
        } else {
          weaponLevel.firingFailed();
        }
      }
    }
  }
}

void GenAI::damage(float amt, float x, float y) noth {
  EmotionSource::damage(amt, x, y);
  if (usedDodgeLastFrame)
    dodge.damage(amt);
}

string GenAI::getScores() const noth {
  ostringstream out;
  #define C(cortex) << cortex.instance << ' ' << cortex.getScore() << ' '
  out
    C(reflex)
    C(avoidEdge)
    C(dodge)
    C(runAway)
    C(frontal)
    C(navigation)
    C(targetAnalysis)
    C(stratWeapon)
    C(aiming)
    C(oppWeapon)
    C(weaponLevel)
    ;
  #undef C
  return out.str();
}
