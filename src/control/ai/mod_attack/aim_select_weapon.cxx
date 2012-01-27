/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_attack/aim_select_weapon.hxx
 */

/*
 * aim_select_weapon.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <cmath>
#include <iostream>
#include <cstdlib>

#include "aim_select_weapon.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

#define WEAPON_CANT_USE (1 << (sizeof(int)*8-1))

AIM_SelectWeapon::AIM_SelectWeapon(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  temperatureCaution(s.exists("temperature_caution")? s["temperature_caution"] : 0.95f),
  minMonophaseSize  (s.exists("min_monophase_size" )? s["min_monophase_size" ] : 0.2f),
  magnetoCaution    (s.exists("magneto_caution"    )? s["magneto_caution"    ] : 2.0f)
{ }

void AIM_SelectWeapon::action() {
  if (!ship->target.ref) return;
  Ship* t = (Ship*)ship->target.ref;

  //Determine how wide an angle we have to hit the target
  float tx = t->getX() + controller.gstat("suggest_target_offset_x", 0.0f);
  float ty = t->getY() + controller.gstat("suggest_target_offset_y", 0.0f);
  float dx = tx - ship->getX();
  float dy = ty - ship->getY();
  float dist = sqrt(dx*dx + dy*dy);
  float maxAngleDiff = fabs(atan(t->getRadius() / dist));
  float angle = atan2(dy, dx) - ship->getRotation();
  float minAngle = angle - maxAngleDiff;
  float maxAngle = angle + maxAngleDiff;
  while (minAngle < 0) minAngle += 2*pi;
  while (maxAngle < 0) maxAngle += 2*pi;
  minAngle = fmod(minAngle, 2*pi);
  maxAngle = fmod(maxAngle, 2*pi);
  if (maxAngle < minAngle) maxAngle += 2*pi;

  /* Because of the above math, it is possible for
   * minAngle and maxAngle > 2*pi. To properly handle this,
   * angle checks should have the form
   *   (angle      > minAngle && angle      < maxAngle) ||
   *   (angle+2*pi > minAngle && angle+2*pi < maxAngle)
   */

  float ratings[AIControl::numWeapons] = {0};

  //While going through the below, count how many launchers
  //are at each angle (rounded to the nearest 15 deg)
  unsigned angles[AIControl::numWeapons][360/15] = { /* init to zero */ };
  //Inspect each weapon type
  for (unsigned i=0; i<AIControl::numWeapons; ++i) {
    float angleBonus, damageBonus;
    float accPenalty, distPenalty;
    switch ((Weapon)i) {
      case Weapon_EnergyCharge:
        angleBonus=0;
        damageBonus=1;
        accPenalty=0;
        distPenalty=0.2f;
        break;
      case Weapon_MagnetoBomb:
        angleBonus=pi/6;
        damageBonus=2;
        accPenalty=0.5f;
        distPenalty=0.5f;
        break;
      case Weapon_PlasmaBurst:
        angleBonus=0;
        damageBonus=4;
        accPenalty=0;
        distPenalty=0.1f;
        break;
      case Weapon_SGBomb:
        angleBonus=pi/4;
        damageBonus=3;
        accPenalty=0.9f;
        distPenalty=0.5f;
        break;
      case Weapon_GatlingPlasma:
        angleBonus=pi/6;
        damageBonus=6;
        accPenalty=0.1f;
        distPenalty=0.15f;
        break;
      case Weapon_Monophase:
        angleBonus=0;
        damageBonus=7;
        accPenalty=0;
        distPenalty=0.1f;
        break;
      case Weapon_Missile:
        angleBonus=pi/3;
        damageBonus=8;
        accPenalty=0.4f;
        distPenalty=0;
        break;
      case Weapon_ParticleBeam:
        cerr << "FATAL: AIM_SelectWeapon doesn't know how to handle Weapon_ParticleBeam" << endl;
        exit(EXIT_PROGRAM_BUG);
    }

    const AIControl::WeaponInfo& info(controller.getWeaponInfo(i));
    if (info.empty()) {
      //Can't use, it doesn't exist
      ratings[i] = WEAPON_CANT_USE;
      continue;
    }

    //Check for possible hits
    for (unsigned j=0; j<info.size(); ++j) {
      if ((info[j].theta      > minAngle-angleBonus && info[j].theta      < maxAngle+angleBonus)
      ||  (info[j].theta+2*pi > minAngle-angleBonus && info[j].theta+2*pi < maxAngle+angleBonus))
        ratings[i] += damageBonus;
      if ((info[j].theta      > minAngle && info[j].theta      < maxAngle)
      ||  (info[j].theta+2*pi > minAngle && info[j].theta+2*pi < maxAngle))
        ratings[i] -= accPenalty*damageBonus;
      //Count angles with 45 degree grace if not forward-facing
      if (((info[j].theta      > minAngle-pi/4 && info[j].theta      < maxAngle+pi/4)
      ||  (info[j].theta+2*pi > minAngle-pi/4 && info[j].theta+2*pi < maxAngle+pi/4))
      &&  (info[j].theta > pi/4 && info[j].theta < 7*pi/4)) {
        ++angles[i][(unsigned)(info[j].theta/pi*180/15 + 0.5f) % (360/15)];
        ratings[i] += damageBonus/3;
      }

      ratings[i] *= max(0.0f, 1.0f - dist*distPenalty);
    }

    if (i == (int)Weapon_PlasmaBurst || i == (int)Weapon_GatlingPlasma) {
      float pct = ship->getHeatPercent();
      if (pct > temperatureCaution)
        ratings[i] = (int)(ratings[i] * (1 - (pct-temperatureCaution)/(1-temperatureCaution)));
    } else if (i == (int)Weapon_Monophase) {
      if (t->getRadius()*2 < minMonophaseSize)
        ratings[i] /= 2;
    } else if (i == (int)Weapon_MagnetoBomb) {
      float f = t->getMass() / (float)ship->getMass() / dist;
      if (f < magnetoCaution)
        ratings[i] = 0;
    }
  }

  //Find the best weapon
  int maxScore = WEAPON_CANT_USE;
  unsigned best = 0;
  for (unsigned i=0; i<AIControl::numWeapons; ++i) {
    if (ratings[i] > maxScore) {
      maxScore = ratings[i];
      best = i;
    }
  }

  controller.setCurrentWeapon(best);

  controller.sstat("no_appropriate_weapon", maxScore <= 0);

  //Find best angle
  unsigned bestAngle=0;
  unsigned bestAngleCount=0;
  for (unsigned i=0; i<360/15; ++i)
    if (angles[best][i] > bestAngleCount) {
      bestAngle = i;
      bestAngleCount = angles[best][i];
    }

  controller.sstat("suggest_angle_offset", (float)(bestAngle/180.0f*15.0f*pi));
}

static AIModuleRegistrar<AIM_SelectWeapon> registrar("attack/select_weapon");
