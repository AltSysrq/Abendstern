/**
 * @file
 * @author Jason Lingle
 * @brief Contains the EnergyChargeLauncher system
 */

#ifndef ENERGY_CHARGE_LAUNCHER_HXX_
#define ENERGY_CHARGE_LAUNCHER_HXX_

#include "src/ship/sys/launcher.hxx"

/** The EnergyChargeLauncher is a simple launcher that emits EnergyCharges.
 * It is Abendstern's original weapon.
 */
class EnergyChargeLauncher: public Launcher {
  public:
  EnergyChargeLauncher(Ship* parent);

  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;
  virtual const char* weapon_getStatus(Weapon) const noth;
  virtual const char* weapon_getComment(Weapon) const noth;
  virtual bool weapon_getWeaponInfo(Weapon, WeaponHUDInfo&) const noth;
  virtual void audio_register() const noth;

  protected:
  virtual float getArmTime() const noth;
  virtual GameObject* createProjectile() noth;
  virtual float getFirePower() const noth;
  public:
  DEFAULT_CLONE(EnergyChargeLauncher)
};

#endif /*ENERGY_CHARGE_LAUNCHER_HXX_*/
