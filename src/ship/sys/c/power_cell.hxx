/**
 * @file
 * @author Jason Lingle
 * @brief Contains the PowerCell system
 */

#ifndef POWER_CELL_HXX_
#define POWER_CELL_HXX_

#include "src/ship/sys/power_plant.hxx"

/** The PowerCell is the stealth power plant.
 * It is available from Class C. It produces little
 * energy, but does not explode when damaged (though it
 * does cease to exist then).
 */
class PowerCell : public PowerPlant {
  public:
  PowerCell(Ship* s);
  virtual int normalPowerUse() const noth;
  virtual unsigned mass() const noth;
  virtual void destroy() noth {}
  virtual bool particleBeamCollision(const ParticleBurst*) noth;
  DEFAULT_CLONE(PowerCell)
};

#endif /*ANTIMATTER_POWER_HXX_*/
