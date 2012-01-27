/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/plasma_burst_launcher.hxx
 */

#include <cmath>
#include <cstdio>

#include <GL/gl.h>

#include "plasma_burst_launcher.hxx"
#include "src/weapon/plasma_burst.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

#define ARM_TIME 100.0f
using namespace std;

PlasmaBurstLauncher::PlasmaBurstLauncher(Ship* parent)
: Launcher(parent, system_texture::plasmaBurstLauncher, Weapon_PlasmaBurst, 1, 100),
  status("plasma_burst")
{ }

PlasmaBurstLauncher::PlasmaBurstLauncher(Ship* parent, Weapon weap, GLuint tex, const char* stat)
: Launcher(parent, tex, weap, 1, 100),
  status(stat)
{ }

unsigned PlasmaBurstLauncher::mass() const noth {
  return 60;
}

signed PlasmaBurstLauncher::normalPowerUse() const noth {
  return 10;
}

float PlasmaBurstLauncher::getArmTime() const noth {
  return ARM_TIME;
}

GameObject* PlasmaBurstLauncher::createProjectile() noth {
  if (parent->soundEffectsEnabled() && audio::plasmaBurstLauncherOK) {
    audio::plasmaBurstLauncherOK = false;
    audio::plasmaBurstLauncher.play(0x7FFF * (1.0f - 1.0f/sqrt((float)energyLevel)));
  }

  parent->physicsRequire(PHYS_SHIP_PBL_INVENTORY_BIT);
  parent->heatInfo.temperature += energyLevel;
  parent->heatInfo.coolRate=0.5f;
  pair<float,float> point=Ship::cellCoord(parent, container);
  pair<float,float> vel=parent->getCellVelocity(container);
  return new PlasmaBurst(parent->getField(), parent, point.first, point.second,
                         vel.first, vel.second, parent->getRotation()+theta, energyLevel);
}

float PlasmaBurstLauncher::getFirePower() const noth {
  return (energyLevel+sqrt((float)energyLevel))*2.2f;
}

bool PlasmaBurstLauncher::weapon_isReady(Weapon clazz) const noth {
  if (clazz != weaponClass) return true;
  return parent->heatInfo.temperature < MAX_TEMP
      && Launcher::weapon_isReady(clazz);
}

const char* PlasmaBurstLauncher::weapon_getStatus(Weapon clazz) const noth {
  float kelvins = parent->heatInfo.temperature;
  if (clazz != weaponClass) return NULL;
  if (kelvins > MAX_TEMP) return "hourglass";
  else return status;
}

const char* PlasmaBurstLauncher::weapon_getComment(Weapon clazz) const noth {
  if (clazz != weaponClass) return NULL;
  float kelvins = parent->heatInfo.temperature;
  static char message[32];
  if (kelvins > MAX_TEMP) {
    static string overheat(_(plasma_burst_launcher,overheat));
    sprintf(message, "\a[(danger)%s\a]",overheat.c_str());
  } else if (kelvins > WARN_TEMP) {
    static string hot(_(plasma_burst_launcher,hot));
    sprintf(message, "\a[(warning)%04d K %s\a", (unsigned)kelvins, hot.c_str());
  } else {
    static string ok(_(plasma_burst_launcher,ok));
    sprintf(message, "%04d K %s", (unsigned)kelvins, ok.c_str());
  }
  return message;
}

bool PlasmaBurstLauncher::weapon_getWeaponInfo(Weapon clazz, WeaponHUDInfo& info) const noth {
  if (clazz != weaponClass) return NULL;
  float kelvins = parent->heatInfo.temperature;
  info.speed = PB_SPEED;
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  info.rightBarVal = energyLevel / (float)maxPower;
  info.rightBarCol = Vec3(1,1,1);
  info.botBarVal = kelvins / MAX_TEMP;
  if (kelvins < WARN_TEMP)
    info.botBarCol = Vec3(1,1,0);
  else
    info.botBarCol = Vec3(1,0,0);
  return true;
}

void PlasmaBurstLauncher::audio_register() const noth {
  audio::plasmaBurstLauncher.addSource(container);
}
