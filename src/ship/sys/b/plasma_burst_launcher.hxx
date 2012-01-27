/**
 * @file
 * @author Jason Lingle
 * @brief Contains the PlasmaBurstLauncher system
 */

#ifndef PLASMA_BURST_LAUNCHER_HXX_
#define PLASMA_BURST_LAUNCHER_HXX_

#include "src/ship/sys/launcher.hxx"
class GameObject;

/**
 * The PlasmaBurstLauncher fires PlasmaBursts, dumping heat into the Ship's
 * resovoir.
 */
class PlasmaBurstLauncher: public Launcher {
  private:
  //Following now stored within Ship::plasmaBurstLauncherInfo
  //Each burst heats the launcher up; it will not fire when hotter
  //than 1 thousand kelvins
  //float kelvins;
  //Cool more quickly as we are used less
  //float coolRate;

  const char* status;

  protected:
  /** Constructs a PlasmaBurst launcher with non-standard parms.
   *
   * @param parent Containing Ship
   * @param wc Weapon instead of Weapon_PlasmaBurst
   * @param tex System texture instead of system_texture::plasmaBurstLauncher
   * @param status Status icon instead of plasma_burst
   */
  PlasmaBurstLauncher(Ship* parent, Weapon wc, GLuint tex, const char* status);

  public:
  PlasmaBurstLauncher(Ship* parent);

  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;

  virtual bool weapon_isReady(Weapon clazz) const noth;
  virtual const char* weapon_getStatus(Weapon clazz) const noth;
  virtual const char* weapon_getComment(Weapon clazz) const noth;
  virtual bool weapon_getWeaponInfo(Weapon, WeaponHUDInfo&) const noth;
  virtual bool heating_count() noth { return true; }
  virtual void audio_register() const noth;

  DEFAULT_CLONEWO(PlasmaBurstLauncher)

  protected:
  virtual float getArmTime() const noth;
  virtual GameObject* createProjectile() noth;
  virtual float getFirePower() const noth;
};


#endif /*PLASMA_BURST_LAUNCHER_HXX_*/
