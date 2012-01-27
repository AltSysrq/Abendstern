/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Heatsink system
 */

#ifndef HEATSINK_HXX_
#define HEATSINK_HXX_

#include "src/ship/sys/ship_system.hxx"

/** A Heatsink is a simple cooling system that provides
 * 0.2 cooling.
 *
 * It is light and requires only minimal
 * energy to maintain.
 */
class Heatsink : public ShipSystem {
  public:
  Heatsink(Ship* par);

  virtual unsigned mass() const noth { return 35; }
  virtual signed normalPowerUse() const noth;
  virtual float cooling_amount() noth { return 0.2f; }
  DEFAULT_CLONE(Heatsink)
};

#endif /*HEATSINK_HXX_*/
