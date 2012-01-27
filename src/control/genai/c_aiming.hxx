/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AimingCortex
 */

/*
 * c_aiming.hxx
 *
 *  Created on: 03.11.2011
 *      Author: jason
 */

#ifndef C_AIMING_HXX_
#define C_AIMING_HXX_

#include "cortex.hxx"
#include "ci_self.hxx"
#include "ci_objective.hxx"
#include "ci_cellt.hxx"
#include "ci_nil.hxx"

class Ship;
class GameObject;

/**
 * The AimingCortex controls the ship to try to fire the strategic weapon.
 */
class AimingCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Objective
<cortex_input::CellT
<cortex_input::Nil> > > {
  Ship*const ship;
  float score;
  signed strategicWeapon;

  //There are 8 outputs with each name base; group by name
  static const unsigned throttle = 0,
                        accel = throttle+8,
                        brake = accel+8,
                        spin = accel+8,
                        numOutputs = spin+8;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new AimingCortex.
   * @param species The root of the data to read from
   * @param s The ship to control
   * @param ss The SelfSource to use
   * @param cs The CellTSource to use
   */
  AimingCortex(const libconfig::Setting& species, Ship* s,
               cortex_input::SelfSource* ss,
               cortex_input::CellTSource* cs);

  /**
   * Evaluates the cortex and controls the Ship, given the elapsed time,
   * the weapon to use (which must be between 0 and 7, inclusive), and
   * the current Objective.
   */
  void evaluate(float,signed,const GameObject*);

  /** To be called whenever a weapon is fired, with the type of weapon
   * that is fired.
   */
  void weaponFired(signed);

  /** Returns the score of the cortex */
  float getScore() const { return score; }
};

#endif /* C_AIMING_HXX_ */
