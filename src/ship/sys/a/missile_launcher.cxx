/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/missile_launcher.hxx
 */

/*
 * missile_launcher.cxx
 *
 *  Created on: 07.03.2011
 *      Author: jason
 */

#include <cmath>

#include "missile_launcher.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/ship/ship.hxx"
#include "src/weapon/missile.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

#define RELOAD_TIME 1024.0f
#define SLOW_FIRE_TIME 500.0f
#define POWER_MUL 50.0f
#define POWER_USE 50.0f

using namespace std;

MissileLauncher::MissileLauncher(Ship* par)
: Launcher(par, system_texture::missileLauncher, Weapon_Missile, 1, MAX_MISSLE_POW)
{ }

const char* MissileLauncher::weapon_getStatus(Weapon weap) const noth {
  if (weap == weaponClass)
    return (::weapon_isReady(parent, weaponClass)? "missile_launcher" : "hourglass");
  else
    return NULL;
}

const char* MissileLauncher::weapon_getComment(Weapon weap) const noth {
  if (weap != weaponClass) return NULL;
  if (::weapon_isReady(parent, weaponClass))
    RETL10N(missile,ready)
  else
    RETL10N(missile,reload)
}

bool MissileLauncher::weapon_getWeaponInfo(Weapon weap, WeaponHUDInfo& info) const noth {
  if (weap != weaponClass) return false;
  info.speed = MISSILE_LAUNCH_SPEED;
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  info.rightBarVal = energyLevel / (float)maxPower;
  info.rightBarCol = Vec3(0.8f,0.8f,1);
  return true;
}

unsigned MissileLauncher::mass() const noth {
  return 150;
}

signed MissileLauncher::normalPowerUse() const noth {
  return POWER_USE;
}

GameObject* MissileLauncher::createProjectile() noth {
  if (audio::missileLauncherOK && parent->soundEffectsEnabled()) {
    audio::missileLauncher.play(0x3FFF + 1638*energyLevel);
    audio::missileLauncherOK = false;
  }

  pair<float,float> coord=Ship::cellCoord(parent, container);
  pair<float,float> baseVel=parent->getCellVelocity(container);
  float angle=theta+parent->getRotation();
  return new Missile(parent->getField(), energyLevel,
                     coord.first, coord.second,
                     baseVel.first  + MISSILE_LAUNCH_SPEED*cos(angle),
                     baseVel.second + MISSILE_LAUNCH_SPEED*sin(angle),
                     parent, parent->target.ref);
}

float MissileLauncher::getArmTime() const noth {
  return RELOAD_TIME;
}

float MissileLauncher::getFirePower() const noth {
  return POWER_MUL*pow((float)energyLevel, 1.5f);
}

void MissileLauncher::audio_register() const noth {
  audio::missileLauncher.addSource(container);
}
