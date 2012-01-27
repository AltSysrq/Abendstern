/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ParticleBeamLauncher system
 */

/*
 * particle_beam_launcher.hxx
 *
 *  Created on: 09.03.2011
 *      Author: jason
 */

#ifndef PARTICLE_BEAM_LAUNCHER_HXX_
#define PARTICLE_BEAM_LAUNCHER_HXX_

#include "src/ship/sys/launcher.hxx"

/** The ParticleBeamLauncher is a Launcher that spawns ParticleEmitters.
 * It maps energy levels to beam types.
 */
class ParticleBeamLauncher: public Launcher {
  public:
  ParticleBeamLauncher(Ship*);
  DEFAULT_CLONEWO(ParticleBeamLauncher)

  virtual const char* weapon_getStatus(Weapon) const noth;
  virtual const char* weapon_getComment(Weapon) const noth;
  virtual bool weapon_getWeaponInfo(Weapon, WeaponHUDInfo&) const noth;
  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;
  virtual void audio_register() const noth;

  protected:
  virtual GameObject* createProjectile() noth;
  virtual float getArmTime() const noth;
  virtual float getFirePower() const noth;
};

#endif /* PARTICLE_BEAM_LAUNCHER_HXX_ */
