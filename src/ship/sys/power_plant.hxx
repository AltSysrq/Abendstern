/**
 * @file
 * @author Jason Lingle
 * @brief Contains the PowerPlant abstract system
 */

#ifndef POWER_PLANT_HXX_
#define POWER_PLANT_HXX_

#include "ship_system.hxx"

/** A PowerPlant is a system that generates power.
 * Subclasses must only specify capacity,
 * drawing, mass, and destroy() behaviour.
 */
class PowerPlant: public ShipSystem {
  friend class TclPowerPlant;
  protected:
  /** Tracks milliseconds until explosion when damaged.
   *
   * When damage() is called, we set this to a random time and
   * set the update function. When the time expires, we call
   * explode().
   */
  float timeUntilExplosion;

  /**
   * Tracks the blame to assign to a blast created by an explosion.
   */
  unsigned explosionBlame;

  /** Constructs a new PowerPlant with the given Ship and system texture */
  PowerPlant(Ship* p, GLuint tex)
  : ShipSystem(p, tex, Classification_Power)
  { }

  public:
  /** Implements setting timeUntilExplosion and adding the update() function.
   * Subclasses are expected to implement destroy().
   */
  virtual bool damage(unsigned) noth;

  /** PowerPlant-specific update function to track timeUntilExplosion.
   * Calls destroy() when that hits zero.
   */
  static void update(ShipSystem*, float) noth;
};
#endif /*POWER_PLANT_HXX_*/
