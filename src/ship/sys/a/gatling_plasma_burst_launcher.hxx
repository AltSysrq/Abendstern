/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GatlingPlasmaBurstLauncher system
 */

/*
 * gatling_plasma_burst_launcher.hxx
 *
 *  Created on: 11.02.2011
 *      Author: jason
 */

#ifndef GATLING_PLASMA_BURST_LAUNCHER_HXX_
#define GATLING_PLASMA_BURST_LAUNCHER_HXX_

#include "src/ship/sys/b/plasma_burst_launcher.hxx"

/** The GatlingPlasmaBurstLauncher is a Class A variant of
 * the PlasmaBurst launcher.
 *
 * It features less heat production and
 * much higher rate of fire at the cost of greater energy
 * consumption and a loss of accuracy. The launcher comes
 * in two variations:
 * + Standard. Fire rate: 3x, energy 3x/3, heating: 1x/3, variance: 5 deg
 * + Turbo.    Fire rate: 6x, energy 4x/3, heating: 3x/3, variance: 10 deg
 *
 * As an added disadvantage, a GatlingPlasmaBurstLauncher will wear out
 * over time. Each shot will add the current temperature percent (where 1
 * would be maximum temperature) squared to its wear level. The arm time
 * is affected by the equation (baseArmTime / (1 - wearPercent^2)). No
 * shots can be fired if wear level is greater than the maximum.
 */
class GatlingPlasmaBurstLauncher: public PlasmaBurstLauncher {
  bool isTurbo;
  float wear;

  public:
  GatlingPlasmaBurstLauncher(Ship*, bool turbo);
  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;

  virtual ShipSystem* clone() noth;

  /** Returns whether the Launcher is set to turbo mode. */
  bool getTurbo() const noth { return isTurbo; }

  virtual bool weapon_isReady(Weapon) const noth;
  virtual const char* weapon_getStatus(Weapon) const noth;
  virtual const char* weapon_getComment(Weapon clazz) const noth;
  virtual bool weapon_getWeaponInfo(Weapon, WeaponHUDInfo&) const noth;
  virtual void audio_register() const noth;

  protected:
  virtual float getArmTime() const noth;
  virtual GameObject* createProjectile() noth;
  virtual float getFirePower() const noth;
};

#endif /* GATLING_PLASMA_BURST_LAUNCHER_HXX_ */
