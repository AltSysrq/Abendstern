/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AntimatterPower system
 */

#ifndef ANTIMATTER_POWER_HXX_
#define ANTIMATTER_POWER_HXX_

#include "src/ship/sys/power_plant.hxx"

/** AntimatterPower is the Class A power system.
 * It provides twice as much power as FusionPower, but
 * explodes far more violently.
 */
class AntimatterPower: public PowerPlant {
  public:
  AntimatterPower(Ship* s);
  virtual signed normalPowerUse() const noth;

  virtual unsigned mass() const noth;
  virtual void destroy(unsigned) noth;
  virtual bool particleBeamCollision(const ParticleBurst*) noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONE(AntimatterPower)
};

#endif /*ANTIMATTER_POWER_HXX_*/
