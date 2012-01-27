/**
 * @file
 * @author Jason Lingle
 * @brief Contains the WeaponLevelCortex
 */

/*
 * c_weapon_level.hxx
 *
 *  Created on: 03.11.2011
 *      Author: jason
 */

#ifndef C_WEAPON_LEVEL_HXX_
#define C_WEAPON_LEVEL_HXX_

#include "cortex.hxx"
#include "ci_self.hxx"
#include "ci_objective.hxx"
#include "ci_cellt.hxx"
#include "ci_weapon.hxx"
#include "ci_nil.hxx"

class Ship;
class GameObject;

/**
 * The WeaponLevelCortex examines each Launcher of a specific type
 * to determine what level to fire the weapon at.
 */
class WeaponLevelCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Objective
<cortex_input::CellT
<cortex_input::Weapon
<cortex_input::Nil> > > > {
  Ship*const ship;
  float score;

  static const unsigned numOutputs = 8;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new WeaponLevelCortex
   * @param species The root of the data to read from
   * @param s The ship to operate on
   * @param ss The SelfSource to use
   * @param cs The CellTSource to use
   */
  WeaponLevelCortex(const libconfig::Setting& species, Ship* s,
                    cortex_input::SelfSource* ss,
                    cortex_input::CellTSource* cs);

  /**
   * Evaluates the cortex and returns the level to use, given the
   * opportunistic weapon and current objective.
   * The weapon must be between 0 and 7 inclusive.
   */
  signed evaluate(signed,const GameObject*);

  /**
   * To be called when damage is dealt to the objective.
   */
  void damageDealt(float);

  /**
   * To be called when firing the weapon bottoms the capacitance out.
   */
  void firingFailed();

  /** Returns the score of the cortex. */
  float getScore() const { return score; }
};

#endif /* C_WEAPON_LEVEL_HXX_ */
