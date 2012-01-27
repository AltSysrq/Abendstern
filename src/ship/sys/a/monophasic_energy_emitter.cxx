/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/monophasic_energy_emitter.hxx
 */

/*
 * monophasic_energy_emitter.cxx
 *
 *  Created on: 12.02.2011
 *      Author: jason
 */

#include <cmath>
#include <cstdio>

#include "monophasic_energy_emitter.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/weapon/monophasic_energy_pulse.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

#define ARM_TIME 125.0f
#define POWER_USE 50

MonophasicEnergyEmitter::MonophasicEnergyEmitter(Ship* parent)
: Launcher(parent, system_texture::monophasicEnergyEmitter, Weapon_Monophase, 1, MAX_MONO_POW)
{
}

const char* MonophasicEnergyEmitter::weapon_getStatus(Weapon w) const noth {
  if (w == weaponClass) return "monophasic";
  else return NULL;
}

const char* MonophasicEnergyEmitter::weapon_getComment(Weapon w) const noth {
  static char msg[32];
  if (w != weaponClass) return NULL;

  static string wavelen(_(monophasic,wavelen));
  sprintf(msg, "%s: %.1f", wavelen.c_str(), MonophasicEnergyPulse::getWavelength(energyLevel)/STD_CELL_SZ);
  return msg;
}

bool MonophasicEnergyEmitter::weapon_getWeaponInfo(Weapon w, WeaponHUDInfo& info) const noth {
  if (w != weaponClass) return NULL;

  info.speed = MonophasicEnergyPulse::getSpeed(energyLevel);
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  info.rightBarVal = energyLevel / (float)maxPower;
  info.rightBarCol = Vec3(1,1,1);
  info.topBarVal = MonophasicEnergyPulse::getSurvivalProbability(energyLevel, info.dist);
  info.topBarCol = Vec3(1-info.topBarVal, 1, info.topBarVal);
  return true;
}

GameObject* MonophasicEnergyEmitter::createProjectile() noth {
  if (parent->soundEffectsEnabled() && audio::monophasicEnergyEmitterOK) {
    audio::monophasicEnergyEmitterOK = false;
    audio::monophasicEnergyEmitter.play(0x3FFF + 0x4000*energyLevel/MAX_MONO_POW);
  }

  pair<float,float> coord = Ship::cellCoord(parent, container);
  return new MonophasicEnergyPulse(parent->getField(), parent,
                                   coord.first, coord.second,
                                   parent->getRotation() + theta,
                                   energyLevel);
}

float MonophasicEnergyEmitter::getArmTime() const noth {
  return ARM_TIME;
}

float MonophasicEnergyEmitter::getFirePower() const noth {
  return energyLevel*25.0f;
}

unsigned MonophasicEnergyEmitter::mass() const noth {
  return 100;
}

signed MonophasicEnergyEmitter::normalPowerUse() const noth {
  return POWER_USE;
}

void MonophasicEnergyEmitter::audio_register() const noth {
  audio::monophasicEnergyEmitter.addSource(container);
}
