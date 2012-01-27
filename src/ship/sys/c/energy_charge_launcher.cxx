/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/energy_charge_launcher.hxx
 */

#include <cmath>
#include <cstdio>
#include <algorithm>

#include <GL/gl.h>

#include "energy_charge_launcher.hxx"
#include "src/weapon/energy_charge.hxx"
#include "src/globals.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"
using namespace std;

#define ARM_TIME 100.0f

/* Instead of displaying nothing for our comment, we'll be
 * more creative and rotate through displays of meaningless
 * information that sounds appropriate. Each message has a
 * base number and a slope; the actual display value is
 * determined with base + slope*cos(x)*sin(2.5*x)/10 + slope*intensity,
 * where x is an arbitrary value computed based on the field clock.
 * The float is floored to an integer before formating.
 */
struct {
  const char* format;
  float base, slope;
} static const comments[] = {
  // 0123456789%3d
  // 01234567890%2d
  // 012345678901%1d
  { "DECAY: %2d BQ" , 80, 5 },
  { "EXORBITAL: %1dF", 4, 2 },
  { "PRES: %3dKPA", 256, 32 },
  { "RESON: %2d THZ", 45, 10 },
  { "CHG: %2d KC", 8, 8 },
  { "POT: %3d KV", 128, 128 },
  { "FLOW: %2d MA", 5, 5 },
  { "FLD: %2d T", 8, 16 },
};

static char comment[14];

EnergyChargeLauncher::EnergyChargeLauncher(Ship* parent)
: Launcher(parent, system_texture::energyChargeLauncher, Weapon_EnergyCharge, 2, 999) {
}

unsigned EnergyChargeLauncher::mass() const noth {
  return 50;
}

signed EnergyChargeLauncher::normalPowerUse() const noth {
  return 1;
}

float EnergyChargeLauncher::getArmTime() const noth {
  return ARM_TIME;
}

GameObject* EnergyChargeLauncher::createProjectile() noth {
  float intensity = 1.0f - 1.0f/(0.91+energyLevel/10.0f);
  if (parent->soundEffectsEnabled() && audio::energyChargeLauncherOK) {
    audio::energyChargeLauncher0.play(0x7FFF);
    audio::energyChargeLauncher1.play(intensity*0x7FFF);
    audio::energyChargeLauncherOK = false;
  }
  pair<float,float> point=Ship::cellCoord(parent, container);
  return new EnergyCharge(parent->getField(), parent,
                          point.first, point.second,
                          parent->getRotation()+theta,
                          intensity);
}

float EnergyChargeLauncher::getFirePower() const noth {
  return energyLevel*2.5f;
}

const char* EnergyChargeLauncher::weapon_getStatus(Weapon cls) const noth {
  if (cls == Weapon_EnergyCharge) return "energy_charge";
  else return NULL;
}

const char* EnergyChargeLauncher::weapon_getComment(Weapon cls) const noth {
  if (cls != Weapon_EnergyCharge) return NULL;
  unsigned i = (parent->getField()->fieldClock / 15000) % lenof(comments);
  float intensity = 1.0f - 1.0f/(0.91+energyLevel/10.0f);
  float x = parent->getField()->fieldClock / 60000.0f;
  sprintf(comment, comments[i].format, (int)(comments[i].base
                                           + cos(x)*sin(2.5f*x)/10.0f*comments[i].slope
                                           + comments[i].slope * intensity));
  return comment;
}

bool EnergyChargeLauncher::weapon_getWeaponInfo(Weapon cls, WeaponHUDInfo& info) const noth {
  if (cls != Weapon_EnergyCharge) return false;
  info.speed = EC_SPEED;
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  info.rightBarVal = 1.0f - 1.0f/(0.91+energyLevel/10.0f);
  info.rightBarCol = Vec3(EnergyCharge::getColourR(info.rightBarVal),
                          EnergyCharge::getColourG(info.rightBarVal),
                          EnergyCharge::getColourB(info.rightBarVal));
  info.topBarVal = max(0.0f, EnergyCharge::getIntensityAt(info.rightBarVal, info.dist));
  info.topBarCol = Vec3(EnergyCharge::getColourR(info.topBarVal),
                        EnergyCharge::getColourG(info.topBarVal),
                        EnergyCharge::getColourB(info.topBarVal));
  return true;
}

void EnergyChargeLauncher::audio_register() const noth {
  audio::energyChargeLauncher0.addSource(container);
  audio::energyChargeLauncher1.addSource(container);
}
