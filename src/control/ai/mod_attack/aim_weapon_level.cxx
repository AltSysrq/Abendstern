/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_attack/aim_weapon_level.hxx
 */

/*
 * aim_weapon_level.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "aim_weapon_level.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

#define RANGE 4.0f

AIM_WeaponLevel::AIM_WeaponLevel(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_WeaponLevel::action() {
  /* Do nothing if no target or no appropriate weapon. */
  if (!ship->target.ref || controller.gstat("no_appropriate_weapon", false)) return;
  // If capacitors are empty, do nothing
  if (ship->getCurrentCapacitance() < 0.1e-9) {
    controller.sstat("no_appropriate_weapon", true);
    return;
  }

  //Determine time until hitting the target
  //Or going out of range, as well as distance
  GameObject* t=ship->target.ref;
  float dx = t->getX() - ship->getX();
  float dy = t->getY() - ship->getY();
  float dist = sqrt(dx*dx + dy*dy);
  if (dist > RANGE) return;

  float rvx = t->getVX() - ship->getVX();
  float rvy = t->getVY() - ship->getVY();
  //Dot product to find speed of closing or separation
  float rspeed = (rvx*dx + rvy*dy)/dist;
  float fireTime;
  if (rspeed < -0.01f)
    //Separating, consider 4.0 our range
    fireTime = (RANGE-dist)/rspeed;
  else if (rspeed > +0.01f)
    //Closing
    fireTime = dist/rspeed;
  else
    //Too slow, just assume 4 seconds
    fireTime = 4000.0f;

  /* We use a random method to find a near-optimal level.
   * The level range is divided into segments. A level is
   * picked at random from each segment and evaluated.
   * The segment with the highest sample is used for the
   * next step. Three levels are picked at random from
   * within the segment and evaluated. The level with the
   * highest score is used.
   */
  Weapon weapon = (Weapon)controller.getCurrentWeapon();
  unsigned levelBase, segmentSize, numSegments=1;
  switch (weapon) {
    case Weapon_EnergyCharge:
      levelBase = 2;
      segmentSize = 8;
      numSegments = 8;
      break;
    case Weapon_MagnetoBomb:
    case Weapon_SGBomb:
      levelBase = 1;
      segmentSize = 1;
      numSegments = (controller.gstat("desparation", false)? 9 : 5);
      break;
    case Weapon_PlasmaBurst:
    case Weapon_GatlingPlasma:
      levelBase = 3;
      segmentSize = 6;
      numSegments = 8;
      break;
    case Weapon_Monophase:
      levelBase = 1;
      segmentSize = 6;
      numSegments = 5;
      break;
    case Weapon_Missile:
      levelBase = 1;
      segmentSize = 2;
      numSegments = 5;
      break;
    case Weapon_ParticleBeam: return; //We don't know to handle this
    default:
      cerr << __FILE__ << ':' << __LINE__
           << ": AIM_WeaponLevel::action(): unknown weapon" << endl;
      exit(EXIT_THE_SKY_IS_FALLING);
  }

  float bestScore=-1;
  //Store the number of the sample we used, so we have an extra
  //for the next step
  unsigned bestSegment=0, bestSegmentSample=3;
  for (unsigned i=0; i<numSegments; ++i) {
    unsigned level = levelBase + segmentSize*i + rand()%segmentSize;
    weapon_setEnergyLevel(ship, weapon, level);
    float score = evalSuitability(level, fireTime);

    if (score > bestScore) {
      bestSegment = i;
      bestSegmentSample = level;
      bestScore = score;
    }
  }

  //Handle special case of size=1 by just using
  //the best level we determined above
  if (segmentSize == 1) {
    weapon_setEnergyLevel(ship, weapon, bestSegmentSample);
    //Nothing more to do
    return;
  }

  for (unsigned i=0; i<3; ++i) {
    unsigned level = levelBase + segmentSize*bestSegment + rand()%segmentSize;
    weapon_setEnergyLevel(ship, weapon, level);
    float score = evalSuitability(level, fireTime);

    if (score > bestScore) {
      bestSegmentSample = level;
      bestScore = score;
    }
  }

  weapon_setEnergyLevel(ship, weapon, bestSegmentSample);
}

float AIM_WeaponLevel::evalSuitability(unsigned lvl, float timeUntilReachTarget) {
  float basePowerReq = weapon_getLaunchEnergy(ship, (Weapon)controller.getCurrentWeapon());
  float powerReqPerShot = basePowerReq * controller.getWeaponInfo(controller.getCurrentWeapon()).size();
  float timeBetweenShots;
  switch ((Weapon)controller.getCurrentWeapon()) {
    case Weapon_EnergyCharge:
    case Weapon_PlasmaBurst:
      timeBetweenShots = 100.0f;
      break;
    case Weapon_MagnetoBomb:
    case Weapon_SGBomb:
      timeBetweenShots = 1000.0f;
      break;
    case Weapon_Monophase:
      timeBetweenShots = 125.0f;
      break;
    case Weapon_GatlingPlasma:
      timeBetweenShots = 30.0f;
      break;
    case Weapon_Missile:
      timeBetweenShots = 500.0f;
      break;
    case Weapon_ParticleBeam:
      timeBetweenShots = 2500.0f;
      break;

    default:
      cerr << __FILE__ << ':' << __LINE__
           << ": Unexpected weapon in AIM_WeaponLevel::evalSuitability()"<<endl;
      exit(EXIT_THE_SKY_IS_FALLING);
  }

  float freePower = ship->getPowerSupply() - ship->getPowerDrain();
  //Convert to ms
  freePower /= 1000;

  float numShotsByCapacitance = ship->getCurrentCapacitance() / powerReqPerShot;
  float numShots;
  //See how many shots this level can produce before we hit zero capacitance
  if (numShotsByCapacitance < 1.0f)
    numShots = numShotsByCapacitance;
  else if (freePower < powerReqPerShot/timeBetweenShots)
    numShots = numShotsByCapacitance +
               freePower*timeBetweenShots/powerReqPerShot;
  else {
    //float a =1.0f, b=0.0f;
    //numShots = a/b; //+Infinity, MSVC-friendly way
    numShots = INFINITY;
  }

  float shootingTime = (numShots-1) * timeBetweenShots;

  if (shootingTime < timeUntilReachTarget)
    return numShots*lvl;
  else
    return (timeUntilReachTarget/timeBetweenShots) * lvl;
}

void AIM_WeaponLevel::dumpEvals(unsigned min, unsigned max, float et) {
  for (unsigned i=min; i<=max; ++i) {
    weapon_setEnergyLevel(ship, (Weapon)controller.getCurrentWeapon(), i);
    cout << i << ": " << evalSuitability(i, et) << endl;
  }
}

static AIModuleRegistrar<AIM_WeaponLevel> registrar("attack/weapon_level");
