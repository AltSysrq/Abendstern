/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Launcher system type
 */

#ifndef LAUNCHER_HXX_
#define LAUNCHER_HXX_

#include "ship_system.hxx"
class GameObject;

/** Launcher is the general system type for weapons launchers.
 *
 * It implements most of the requirements for such systems; specifically,
 * a subclass must only minimally do the following:
 * + Implement createProjectile(), getArmTime(), getFirePower()
 * + Override weapon_getComment() and weapon_getWeaponInfo()
 */
class Launcher: public ShipSystem {
  friend class TclLauncher;
  protected:
  /** The Weapon type */
  const Weapon className;
  /** The current energy level */
  int energyLevel;
  /** Angle in radians relative to Ship to fire */
  float theta;
  /** When fieldClock > this value, Launcher can fire */
  Uint32 readyAt;

  int minPower, ///< The minimum energy level, inclusive
      maxPower; ///< The maximum energy level, inclusive

  /** The orientation, representing which container face
   * the Launcher shoots out of.
   */
  int orientation;

  /** Constructs a new Launcher with the given parms.
   *
   * @param parent Ship that contains the Launcher
   * @param tex System texture to use
   * @param _className Name of Weapon type
   * @param min Minimum energy level, inclusive
   * @param max Maximum energy level, inclusive
   */
  Launcher(Ship* parent, GLuint tex, Weapon _className, int min, int max);

  public:
  /** Returns the angle at which projectiles will be launched,
   * relative to the Ship. */
  float getLaunchAngle() const noth;

  virtual const char* autoOrient() noth;
  virtual const char* setOrientation(int) noth;
  virtual int getOrientation() const noth;

  virtual void weapon_enumerate(Weapon, std::vector<ShipSystem*>&) noth;
  virtual int weapon_getEnergyLevel(Weapon) const noth;
  virtual int weapon_getMinEnergyLevel(Weapon) const noth;
  virtual int weapon_getMaxEnergyLevel(Weapon) const noth;
  virtual bool weapon_isReady(Weapon) const noth;
  virtual void weapon_setEnergyLevel(Weapon, int) noth;
  virtual float weapon_getLaunchEnergy(Weapon) const noth;
  virtual void weapon_fire(Weapon) noth;

  protected:
  /** Creates a new projectile and returns it.
   *
   * @return The new projectile, or NULL if the operation could
   * not be completed.
   */
  virtual GameObject* createProjectile() noth = 0;
  /** Returns the time in milliseconds between successive shots. */
  virtual float getArmTime() const noth = 0;
  /** Returns the energy required to fire a shot from one Launcher. */
  virtual float getFirePower() const noth = 0;
};

#endif /*LAUNCHER_HXX_*/
