/**
 * @file
 * @author Jason Lingle
 * @brief Implements src/ship/sys/a/particle_beam_launcher.hxx
 */

/*
 * particle_beam_launcher.cxx
 *
 *  Created on: 09.03.2011
 *      Author: jason
 */

#include <cmath>
#include <cstdio>
#include <algorithm>

#include "particle_beam_launcher.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/weapon/particle_beam.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

#define ARM_TIME 1000.0f
#define POWER_USE 80
#define LAUNCH_SPEED 0.0015f
#define FIRE_POWER 100.0f

ParticleBeamLauncher::ParticleBeamLauncher(Ship* par)
: Launcher(par, system_texture::particleBeamLauncher, Weapon_ParticleBeam, 1, 4)
{ }

const char* ParticleBeamLauncher::weapon_getStatus(Weapon weap) const noth {
  if (weap != weaponClass) return NULL;
  return weapon_isReady(weap)? "particle_beam" : "hourglass";
}

const char* ParticleBeamLauncher::weapon_getComment(Weapon weap) const noth {
  if (weap != weaponClass) return NULL;
  switch ((ParticleBeamType)(energyLevel-1)) {
    case NeutronBeam:    RETL10N(particle_beam,neutron);
    case MuonBeam:       RETL10N(particle_beam,muon);
    case AntiprotonBeam: RETL10N(particle_beam,antiproton);
    case PositronBeam:   RETL10N(particle_beam,positron);
    default: return "\a[(danger)ERROR\a]";
  }
}

bool ParticleBeamLauncher::weapon_getWeaponInfo(Weapon weap, WeaponHUDInfo& info) const noth {
  if (weap != weaponClass) return false;
  info.speed=LAUNCH_SPEED;
  info.leftBarVal = parent->getCapacitancePercent();
  info.leftBarCol = Vec3(1,1,1);
  //Indicate the "energy level" with colour alone, as actual
  //assignments are meaningless relative to each other
  info.rightBarVal = 1.0f;
  switch ((ParticleBeamType)(energyLevel-1)) {
    case NeutronBeam:   info.rightBarCol = Vec3(0,1,0); break;
    case MuonBeam:      info.rightBarCol = Vec3(0,0,1); break;
    case AntiprotonBeam:info.rightBarCol = Vec3(1,0,0); break;
    case PositronBeam:  info.rightBarCol = Vec3(1,0,1); break;
  }
  info.topBarVal = max(0.0f, 1.0f - info.dist/LAUNCH_SPEED/ParticleEmitter::lifetime);
  info.topBarCol = Vec3(1-info.topBarVal, info.topBarVal, 0);
  return true;
}

unsigned ParticleBeamLauncher::mass() const noth {
  return 150;
}

signed ParticleBeamLauncher::normalPowerUse() const noth {
  return POWER_USE;
}

GameObject* ParticleBeamLauncher::createProjectile() noth {
  if (parent->soundEffectsEnabled() && audio::particleBeamLauncherOK) {
    audio::particleBeamLauncherOK = false;
    audio::particleBeamLauncher.play(0x7FFF);
  }

  pair<float,float> coord=Ship::cellCoord(parent, container);
  pair<float,float> baseVel=parent->getCellVelocity(container);
  float angle=theta+parent->getRotation();
  return new ParticleEmitter(parent->getField(), (ParticleBeamType)(energyLevel-1),
                             coord.first, coord.second,
                             baseVel.first  + LAUNCH_SPEED*cos(angle),
                             baseVel.second + LAUNCH_SPEED*sin(angle),
                             parent);
}

float ParticleBeamLauncher::getArmTime() const noth {
  return ARM_TIME;
}

float ParticleBeamLauncher::getFirePower() const noth {
  return FIRE_POWER;
}

void ParticleBeamLauncher::audio_register() const noth {
  audio::particleBeamLauncher.addSource(container);
}
