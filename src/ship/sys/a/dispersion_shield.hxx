/**
 * @file
 * @author Jason Lingle
 * @brief Contains the DispersionShield system
 */

/*
 * dispersion_shield.hxx
 *
 *  Created on: 13.02.2011
 *      Author: jason
 */

#ifndef DISPERSION_SHIELD_HXX_
#define DISPERSION_SHIELD_HXX_

#include "src/ship/sys/ship_system.hxx"

/**
 * DispersionShields absorb damage from the cells near them.
 *
 * The DispersionShield acts only as a flag of sorts to the
 * Ship that such a system exists. All logic for actual dispersion
 * shielding is handled within Ship. This class merely handles the
 * basic properties.
 */
class DispersionShield: public ShipSystem {
  bool isActive;

  public:
  /** Constructs a DispersionShied for the given Ship. */
  DispersionShield(Ship*);
  DEFAULT_CLONE(DispersionShield);

  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth;

  virtual void disperser_enumerate(std::vector<Cell*>&) noth;
  virtual bool heating_count() noth { return true; }
  virtual void shield_deactivate() noth;
};

#endif /* DISPERSION_SHIELD_HXX_ */
