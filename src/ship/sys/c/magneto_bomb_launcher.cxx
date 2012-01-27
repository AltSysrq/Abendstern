/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/magneto_bomb_launcher.hxx
 */

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <GL/gl.h>

#include "magneto_bomb_launcher.hxx"
#include "src/weapon/magneto_bomb.hxx"
#include "src/globals.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"
using namespace std;

#define ARM_TIME 500
#define OVERPOW_LIM 5
#define OVERPOW_ARM_TIME 1500
#define LAUNCH_SPEED MB_SPEED

MagnetoBombLauncher::MagnetoBombLauncher(Ship* parent)
: Launcher(parent, system_texture::magnetoBombLauncher, Weapon_MagnetoBomb, 1, 9)
{
  _powerUsage=2;
}

MagnetoBombLauncher::MagnetoBombLauncher(Ship* parent, GLuint tex, Weapon clazz, int pwr)
: Launcher(parent, tex, clazz, 1, 9)
{
  _powerUsage = pwr;
}


unsigned MagnetoBombLauncher::mass() const noth {
  return 80;
}

signed MagnetoBombLauncher::normalPowerUse() const noth {
  return _powerUsage;
}

float MagnetoBombLauncher::getArmTime() const noth {
  if (energyLevel <= OVERPOW_LIM)
    return ARM_TIME;
  else
    return OVERPOW_ARM_TIME;
}

float MagnetoBombLauncher::getFirePower() const noth {
  //Besides the energy for the bomb itself, there
  //is a minimum 10 penalty plus 10 times the
  //square root of the energy
  return 40.0f*energyLevel +
         10.0f +
         10.0f*sqrt((float)energyLevel*10.0f);
}

GameObject* MagnetoBombLauncher::createProjectile() noth {
  if (parent->soundEffectsEnabled() && audio::magnetoBombLauncherOK) {
    audio::magnetoBombLauncherOK = false;
    audio::magnetoBombLauncher0.play(0x7FFF);
    audio::magnetoBombLauncher1.play(energyLevel <= 5? 6553*energyLevel : 0x7FFF);
    if (energyLevel > 5)
      audio::magnetoBombLauncherOP.play((energyLevel-5)*0x1FFF);
  }

  pair<float,float> coord=Ship::cellCoord(parent, container);
  pair<float,float> baseVel=parent->getCellVelocity(container);
  float angle=theta+parent->getRotation();
  MagnetoBomb* ret=new MagnetoBomb(parent->getField(), coord.first, coord.second,
                                   baseVel.first  + LAUNCH_SPEED*cos(angle),
                                   baseVel.second + LAUNCH_SPEED*sin(angle),
                                   energyLevel*MBL_POW_MULT, parent);
  if (shouldFail()) ret->simulateFailure();
  return ret;
}

const char* MagnetoBombLauncher::weapon_getStatus(Weapon weap) const noth {
  if (weap != weaponClass) return NULL;
  else if (parent->getField()->fieldClock < readyAt) return "hourglass";
  else return "magneto_bomb";
}

bool MagnetoBombLauncher::weapon_getWeaponInfo(Weapon weap, WeaponHUDInfo& info) const noth {
  if (weap != weaponClass) return false;
  info.speed = LAUNCH_SPEED;
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  info.rightBarVal = energyLevel / (float)maxPower;
  if (energyLevel > OVERPOW_LIM)
    info.rightBarCol = Vec3(1,0,0);
  else
    info.rightBarCol = Vec3(1,1,1);
  return true;
}

const char* MagnetoBombLauncher::weapon_getComment(Weapon weap) const noth {
  if (weap != weaponClass) return NULL;
  if (parent->getField()->fieldClock < readyAt)
    RETL10N(magneto_bomb,reload)
  else if (energyLevel > OVERPOW_LIM)
    RETL10N(magneto_bomb,overload)
  else
    RETL10N(magneto_bomb,ready)
}

bool MagnetoBombLauncher::shouldFail() const noth {
  if (energyLevel <= OVERPOW_LIM) return false;

  return (rand()%10 < energyLevel-OVERPOW_LIM);
}

void MagnetoBombLauncher::audio_register() const noth {
  audio::magnetoBombLauncher0.addSource(container);
  audio::magnetoBombLauncher1.addSource(container);
  audio::magnetoBombLauncherOP.addSource(container);
}
