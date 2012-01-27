/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/gatling_plasma_burst_launcher.hxx
 */

/*
 * gatling_plasma_burst_launcher.cxx
 *
 *  Created on: 11.02.2011
 *      Author: jason
 */

#include <cstdlib>
#include <algorithm>

#include "gatling_plasma_burst_launcher.hxx"
#include "src/ship/ship.hxx"
#include "src/weapon/plasma_burst.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

#define MASS 120
#define TURBO_POW_USE 80
#define STD_POW_USE 40

#define STD_ARM_TIME 33.3f
#define TURBO_ARM_TIME 16.6f

#define STD_HEAT_MUL 1/3.0f
#define TURBO_HEAT_MUL 1.0f

#define STD_VARIANCE (5/180.0f*pi)
#define TURBO_VARIANCE (15/180.0f*pi)

#define STD_POW_MUL 1
#define TURBO_POW_MUL 1.25f

#define MAX_WEAR 1000.0f

GatlingPlasmaBurstLauncher::GatlingPlasmaBurstLauncher(Ship* parent, bool turbo)
: PlasmaBurstLauncher(parent, Weapon_GatlingPlasma, system_texture::gatlingPlasmaBurstLauncher,
                      "gatling_plasma_burst"),
  isTurbo(turbo), wear(0)
{
}

unsigned GatlingPlasmaBurstLauncher::mass() const noth {
  return MASS;
}

signed GatlingPlasmaBurstLauncher::normalPowerUse() const noth {
  return (isTurbo? TURBO_POW_USE : STD_POW_USE);
}

ShipSystem* GatlingPlasmaBurstLauncher::clone() noth {
  GatlingPlasmaBurstLauncher* gpbl = new GatlingPlasmaBurstLauncher(parent, isTurbo);
  gpbl->setOrientation(getOrientation());
  return gpbl;
}

float GatlingPlasmaBurstLauncher::getArmTime() const noth {
  float wearPercent = wear/MAX_WEAR;
  return (isTurbo? TURBO_ARM_TIME : STD_ARM_TIME) / max(0.1f, (1 - wearPercent*wearPercent));
}

float GatlingPlasmaBurstLauncher::getFirePower() const noth {
  float pow = PlasmaBurstLauncher::getFirePower();
  return pow * (isTurbo? STD_POW_MUL : TURBO_POW_MUL);
}

GameObject* GatlingPlasmaBurstLauncher::createProjectile() noth {
  if (wear > MAX_WEAR) return NULL;

  float tempPercent = parent->heatInfo.temperature / MAX_TEMP;
  wear += tempPercent * tempPercent;

  parent->physicsRequire(PHYS_SHIP_PBL_INVENTORY_BIT);
  parent->heatInfo.temperature +=
      energyLevel/(float)parent->plasmaBurstLauncherInfo.count
      * (isTurbo? TURBO_HEAT_MUL : STD_HEAT_MUL);
  parent->heatInfo.coolRate=0.5f;
  pair<float,float> point=Ship::cellCoord(parent, container);
  pair<float,float> vel=parent->getCellVelocity(container);

  float var = isTurbo? TURBO_VARIANCE : STD_VARIANCE;
  float th = theta + rand()/(float)RAND_MAX*var - var/2;

  if (parent->soundEffectsEnabled()) {
    if (isTurbo) {
      if (audio::gatPlasmaBurstLauncherTurboOK) {
        audio::gatPlasmaBurstLauncherTurboOK = false;
        audio::gatPlasmaBurstLauncherTurbo.play(0x7FFF * (1.0f - 1.0f/sqrt((float)energyLevel)));
      }
    } else {
      if (audio::gatPlasmaBurstLauncherNormOK) {
        audio::gatPlasmaBurstLauncherNormOK = false;
        audio::gatPlasmaBurstLauncherNorm.play(0x7FFF * (1.0f - 1.0f/sqrt((float)energyLevel)));
      }
    }
  }

  return new PlasmaBurst(parent->getField(), parent, point.first, point.second,
                         vel.first, vel.second, parent->getRotation()+th, energyLevel);
}

const char* GatlingPlasmaBurstLauncher::weapon_getStatus(Weapon w) const noth {
  if (w != weaponClass) return NULL;
  if (wear > MAX_WEAR) return "hourglass";
  return PlasmaBurstLauncher::weapon_getStatus(w);
}

bool GatlingPlasmaBurstLauncher::weapon_isReady(Weapon w) const noth {
  return wear <= MAX_WEAR && PlasmaBurstLauncher::weapon_isReady(w);
}

const char* GatlingPlasmaBurstLauncher::weapon_getComment(Weapon w) const noth {
  if (w != weaponClass) return NULL;
  if (wear <= MAX_WEAR) return PlasmaBurstLauncher::weapon_getComment(w);
  if (parent->getField()->fieldClock / 1000 % 2)
    RETL10N(launcher,maint_reqd_0)
  else
    RETL10N(launcher,maint_reqd_1)
}

bool GatlingPlasmaBurstLauncher::weapon_getWeaponInfo(Weapon w, WeaponHUDInfo& info) const noth {
  if (w != weaponClass) return NULL;
  PlasmaBurstLauncher::weapon_getWeaponInfo(w, info);
  info.topBarVal = wear / MAX_WEAR;
  if (wear < MAX_WEAR/2)
    info.topBarCol = Vec3(0,1,1);
  else if (wear < MAX_WEAR*3/4)
    info.topBarCol = Vec3(1,0,1);
  else
    info.topBarCol = Vec3(1,0,0);

  return true;
}

void GatlingPlasmaBurstLauncher::audio_register() const noth {
  if (isTurbo) {
    audio::gatPlasmaBurstLauncherTurbo.addSource(container);
    audio::gatPlasmaBurstLauncherRevTurbo.addSource(container);
  } else {
    audio::gatPlasmaBurstLauncherNorm.addSource(container);
    audio::gatPlasmaBurstLauncherRevNorm.addSource(container);
  }
}
