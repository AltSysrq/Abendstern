/**
 * @file
 * @author Jason Lingle
 * @brief Contains the FusionPower system
 */

#ifndef FUSION_POWER_HXX_
#define FUSION_POWER_HXX_

#include "src/ship/sys/power_plant.hxx"

/** FusionPower provides more energy than FissionPower, but explodes with
 * more force and weighs more.
 */
class FusionPower: public PowerPlant {
  public:
  FusionPower(Ship*);
  virtual signed normalPowerUse() const noth;
  virtual unsigned mass() const noth;
  virtual void destroy(unsigned) noth;
  virtual bool particleBeamCollision(const ParticleBurst*) noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONE(FusionPower)
};

#endif /*FUSION_POWER_HXX_*/
