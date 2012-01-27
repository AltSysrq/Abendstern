/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MissileLauncher system
 */

/*
 * missile_launcher.hxx
 *
 *  Created on: 07.03.2011
 *      Author: jason
 */

#ifndef MISSILE_LAUNCHER_HXX_
#define MISSILE_LAUNCHER_HXX_

#include "src/ship/sys/launcher.hxx"

/** The maximum energy level for Missiles */
#define MAX_MISSLE_POW 10

/** The MissileLauncher is a simple launcher
 * than fires missiles.
 */
class MissileLauncher: public Launcher {
  public:
  MissileLauncher(Ship*);
  DEFAULT_CLONEWO(MissileLauncher)

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

#endif /* MISSILE_LAUNCHER_HXX_ */
