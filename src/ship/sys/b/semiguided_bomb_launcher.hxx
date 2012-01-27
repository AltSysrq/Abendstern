/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SemiguidedBombLauncher system
 */

#ifndef SEMIGUIDED_BOMB_LAUNCHER_HXX_
#define SEMIGUIDED_BOMB_LAUNCHER_HXX_

#include "src/ship/sys/c/magneto_bomb_launcher.hxx"

/** The SemiguidedBombLauncher extends MagnetoBombLauncher to shoot SemiguidedBombs
 * and to do so at higher energy.
 */
class SemiguidedBombLauncher: public MagnetoBombLauncher {
  public:
  SemiguidedBombLauncher(Ship* parent);
  virtual const char* weapon_getStatus(Weapon) const noth;
  virtual void audio_register() const noth;

  DEFAULT_CLONE(SemiguidedBombLauncher)

  protected:
  virtual GameObject* createProjectile() noth;
};

#endif /*SEMIGUIDED_BOMB_LAUNCHER_HXX_*/
