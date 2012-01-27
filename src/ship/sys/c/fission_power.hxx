/**
 * @file
 * @author Jason Lingle
 * @brief Contains the FissionPower system
 */

#ifndef FISSION_POWER_HXX_
#define FISSION_POWER_HXX_

#include "src/ship/sys/power_plant.hxx"

/**
 * FissionPower provides a small amount of power (though the most of
 * Class-C) and explodes moderately on damage.
 */
class FissionPower: public PowerPlant {
  public:
  FissionPower(Ship* s);
  virtual signed normalPowerUse() const noth;
  virtual unsigned mass() const noth;
  virtual void destroy(unsigned) noth;
  virtual bool particleBeamCollision(const ParticleBurst*) noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONE(FissionPower)
};

#endif /*FISSION_POWER_HXX_*/
