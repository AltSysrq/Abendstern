/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ShieldGenerator class
 */

#ifndef SHIELD_GENERATOR_HXX_
#define SHIELD_GENERATOR_HXX_

#include <vector>

#include "src/ship/sys/ship_system.hxx"
#include "src/ship/auxobj/shield.hxx"

/** The ShieldGenerator attaches and matains a Shield to the Ship. */
class ShieldGenerator: public ShipSystem {
  private:
  Shield* shield;
  float _strength, _radius;

  //When dead, stop rotation here
  float deathPoint;
  bool dead;

  public:
  ShieldGenerator(Ship*, float strength=MIN_SHIELD_STR, float radius=MIN_SHIELD_RAD);
  virtual ~ShieldGenerator() { if (shield) /* shield->parent=NULL; */ delete shield; }

  virtual void detectPhysics() noth;
  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;

  /** Returns the maximum strength of the Shield */
  float getStrength() const noth { return _strength; }
  /** Returns the maximum radius of the Shield */
  float getRadius() const noth { return _radius; }
  /** Returns whether the Shield is functioning */
  bool isActive() const noth { return !dead; }

  virtual void shield_enumerate(std::vector<Shield*>&) noth;
  virtual void shield_deactivate() noth;
  virtual void shield_elucidate() noth;

  virtual void audio_register() const noth;

  virtual void write(std::ostream&) noth;
  virtual ShipSystem* clone() noth;

  //For use by the networking system only
  //All values are percents
  /** Internal to networking */
  void setShieldStability(float) noth;
  /** Internal to networking */
  void setShieldStrength(float) noth;
  /** Internal to networking */
  void setShieldAlpha(float) noth;
  /** Returns the current stability of the Shield */
  float getShieldStability() const noth;
  /** Returns the current strength of the shield */
  float getShieldStrength() const noth;
  /** Returns the current alpha of the shield */
  float getShieldAlpha() const noth;
};

#endif /*SHIELD_GENERATOR_HXX_*/
