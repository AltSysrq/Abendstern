/**
 * @file
 * @author Jason Lingle
 * @brief Contains the OppWeaponCortex
 */

/*
 * c_opp_weapon.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_OPP_WEAPON_HXX_
#define C_OPP_WEAPON_HXX_

#include "c_weapon.hxx"

/**
 * The Opp[ortunistic]WeaponCortex extends WeaponCortex with the scoring
 * system perscribed in cortexai.txt.
 */
class OppWeaponCortex: public WeaponCortex {
  float score;
  unsigned nshots;

  public:
  /**
   * Constructs a new OppWeaponCortex
   * @param species The root of the data to read from
   * @param s The Ship to operate on
   * @param ss The SelfSource to use
   * @param cs The CellTSource to use
   */
  OppWeaponCortex(const libconfig::Setting& species, Ship* s,
                  cortex_input::SelfSource* ss, cortex_input::CellTSource* cs);

  /** To be called whenever damage is dealt to the objective. */
  void damageDealt(float);

  /**
   * To be called whenever a weapon is fired /successfully/.
   */
  void weaponFired() { ++nshots; }

  /** Returns the score of the cortex. */
  float getScore() const { return score/(nshots+1); }
};

#endif /* C_OPP_WEAPON_HXX_ */
