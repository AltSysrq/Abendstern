/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/semiguided_bomb_launcher.hxx
 */

#include <cmath>
#include <utility>

#include "semiguided_bomb_launcher.hxx"
#include "src/weapon/semiguided_bomb.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"

#define LAUNCH_SPEED 0.0008f

/** @see src/weapon/semiguided_bomb.hxx */
const float semiguidedBombSpeed = LAUNCH_SPEED;

using namespace std;

SemiguidedBombLauncher::SemiguidedBombLauncher(Ship* s)
: MagnetoBombLauncher(s, system_texture::semiguidedBombLauncher, Weapon_SGBomb, 15)
{}

GameObject* SemiguidedBombLauncher::createProjectile() noth {
  if (parent->soundEffectsEnabled() && audio::sgBombLauncherOK) {
    audio::sgBombLauncherOK = false;
    audio::sgBombLauncher0.play(0x7FFF);
    audio::sgBombLauncher2.play(0x7FFF);
    audio::sgBombLauncher1.play(energyLevel <= 5? 6553*energyLevel : 0x7FFF);
    if (energyLevel > 5)
      audio::sgBombLauncherOP.play((energyLevel-5)*0x1FFF);
  }

  pair<float,float> coord=Ship::cellCoord(parent, container);
  pair<float,float> baseVel=parent->getCellVelocity(container);
  float angle=theta+parent->getRotation();
  SemiguidedBomb* ret = new SemiguidedBomb(parent->getField(), coord.first, coord.second,
                                           baseVel.first  + LAUNCH_SPEED*cos(angle),
                                           baseVel.second + LAUNCH_SPEED*sin(angle),
                                           energyLevel*MBL_POW_MULT, parent);
  if (shouldFail()) ret->simulateFailure();
  return ret;
}

const char* SemiguidedBombLauncher::weapon_getStatus(Weapon w) const noth {
  if (w != weaponClass) return NULL;
  else if (parent->getField()->fieldClock < readyAt) return "hourglass";
  else return "semiguided_bomb";
}

void SemiguidedBombLauncher::audio_register() const noth {
  audio::sgBombLauncher0.addSource(container);
  audio::sgBombLauncher1.addSource(container);
  audio::sgBombLauncher2.addSource(container);
  audio::sgBombLauncherOP.addSource(container);
}
