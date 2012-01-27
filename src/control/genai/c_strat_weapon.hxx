/**
 * @file
 * @author Jason Lingle
 * @brief Contains the StratWeaponCortex
 */

/*
 * c_strat_weapon.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_STRAT_WEAPON_HXX_
#define C_STRAT_WEAPON_HXX_

#include "c_weapon.hxx"

/**
 * The Strat[egic]WeaponCortex extends the WeaponCortex with the scoring
 * system described in cortexai.txt.
 */
class StratWeaponCortex: public WeaponCortex {
  float score;
  signed requestedWeapon;

  public:
  /**
   * Constructs a new StratWeaponCortex
   * @param species The root of the species data to read from
   * @param s The ship to operate on
   * @param ss The SelfSource to use
   * @param cs The CellTSource to use
   */
  StratWeaponCortex(const libconfig::Setting& species, Ship* s,
                    cortex_input::SelfSource* ss, cortex_input::CellTSource* cs);

  /** See WeaponCortex::evaluate() */
  signed evaluate(const GameObject*);

  /** Returns the score of the cortex. */
  float getScore() const { return score; }

  /** To be called whenever a weapon is fired.
   * This maintains the score.
   */
  void weaponFired(signed);
};

#endif /* C_STRAT_WEAPON_HXX_ */
