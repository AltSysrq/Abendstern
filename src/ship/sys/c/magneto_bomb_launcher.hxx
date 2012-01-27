/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MagnetoBombLauncher system
 */

#ifndef MAGNETO_BOMB_LAUNCHER_HXX_
#define MAGNETO_BOMB_LAUNCHER_HXX_

#include "src/ship/sys/launcher.hxx"

/** Energy level power multiplier.
 *
 * Ie, internalEnergyLevel = MBL_POW_MULT*energyLevel
 */
#define MBL_POW_MULT 10.0f

/** The MagnetoBombLauncher is a simple launcher that emits MagnetoBombs.
 *
 * It can be overpowered, but that yields the possibility of random failures.
 */
class MagnetoBombLauncher: public Launcher {
  protected:
  signed _powerUsage;
  MagnetoBombLauncher(Ship* parent, GLuint tex, Weapon clazz, int pwr);

  public:
  MagnetoBombLauncher(Ship* parent);

  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;

  virtual const char* weapon_getStatus(Weapon) const noth;
  virtual const char* weapon_getComment(Weapon) const noth;
  virtual bool weapon_getWeaponInfo(Weapon, WeaponHUDInfo&) const noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONE(MagnetoBombLauncher)

  protected:
  virtual float getArmTime() const noth;
  virtual float getFirePower() const noth;
  virtual GameObject* createProjectile() noth;

  /** Randomly determines whether the next MagnetoBomb or SemiguidedBomb should
   * spontaneously explode when launched due to overpowering.
   */
  bool shouldFail() const noth;
};

#endif /*MAGNETO_BOMB_LAUNCHER_HXX_*/
