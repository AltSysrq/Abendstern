/**
 * @file
 * @author Jason Lingle
 * @brief Contains the WeaponCortex base class
 */

/*
 * c_weapon.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_WEAPON_HXX_
#define C_WEAPON_HXX_

#include "cortex.hxx"
#include "ci_nil.hxx"
#include "ci_self.hxx"
#include "ci_objective.hxx"
#include "ci_weapon.hxx"
#include "ci_cellt.hxx"

class GameObject;

/**
 * The WeaponCortex class is the base class for StrategicWeaponCortex
 * and OpportunisticWeaponCortex. It has all functionality of both, excepting
 * a scoring system.
 */
class WeaponCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Objective
<cortex_input::CellT
<cortex_input::Weapon
<cortex_input::Nil> > > > {
  bool enableNoFire;

  //Outputs are 0..7
  static const unsigned numOutputs = 8;

  public:
  typedef cip_input_map input_map;

  protected:
  /** The ship the cortex operates on. */
  Ship*const ship;
  /**
   * Constructs a new WeaponCortex
   * @param species The root of the data to read from
   * @param cname The name of the cortex type
   * @param s The Ship to operate on
   * @param ss The SelfSource to use
   * @param cs The CellTSource to use
   * @param noFire If true, evaluate() will return -1 if all outputs
   *    return less than zero.
   */
  WeaponCortex(const libconfig::Setting& species, const char* cname, Ship* s,
               cortex_input::SelfSource* ss, cortex_input::CellTSource* cs,
               bool noFire);

  public:
  /**
   * Evaluates the cortex and returns an integer corresponding to
   * a Weapon, or -1 if no weapon should be fired.
   *
   * Note that -1 may even be returned when enableNoFire is false,
   * which will happen if the Ship has no weapons left.
   *
   * @param objective The objective to use for objective input
   */
  signed evaluate(const GameObject* objective);
};

#endif /* C_WEAPON_HXX_ */
