/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MonophasicEnergyEmitter system
 */

/*
 * monophasic_energy_emitter.hxx
 *
 *  Created on: 12.02.2011
 *      Author: jason
 */

#ifndef MONOPHASIC_ENERGY_EMITTER_HXX_
#define MONOPHASIC_ENERGY_EMITTER_HXX_

#include "src/ship/sys/launcher.hxx"

#define MAX_MONO_POW 32

/** The MonophasicEnergyEmitter is a class A system that launches
 * MonophasicEnergyPulses.
 */
class MonophasicEnergyEmitter: public Launcher {
  public:
  MonophasicEnergyEmitter(Ship*);

  DEFAULT_CLONEWO(MonophasicEnergyEmitter)

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

#endif /* MONOPHASIC_ENERGY_EMITTER_HXX_ */
