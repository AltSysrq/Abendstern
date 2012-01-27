/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_WeaponLevel AI module
 */

/*
 * aim_weapon_level.hxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#ifndef AIM_WEAPON_LEVEL_HXX_
#define AIM_WEAPON_LEVEL_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** attack/weapon_level: Determines the best energy level for the current weapon.
 * Weapons that can be overloaded will normally not be, unless
 * the desparation state variable is set to true.
 *
 * @section svars State Variables
 * <table>
 * <tr>
 *   <td>desparation</td>
 *   <td>bool</td>
 *   <td>If true, enables overloading. Defaults to false.</td>
 * </tr><tr>
 *   <td>no_appropriate_weapon</td>
 *   <td>bool</td>
 *   <td>If true, module does nothing. Will set to true if capacitance hits zero.
 *       Defaults to false.</td>
 * </tr>
 * </table>
 */
class AIM_WeaponLevel: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_WeaponLevel(AIControl*, const libconfig::Setting&);
  virtual void action();

  private:
  float evalSuitability(unsigned, float timeUntilReachTarget);
  void dumpEvals(unsigned min, unsigned max, float time);
};

#endif /* AIM_WEAPON_LEVEL_HXX_ */
