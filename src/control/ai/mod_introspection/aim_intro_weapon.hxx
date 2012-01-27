/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_IntroWeapon AI module
 */
/*
 * aim_intro_weapon.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_INTRO_WEAPON_HXX_
#define AIM_INTRO_WEAPON_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** introspection/weapons: Gathers information about one weapon when run, then
 * increments weapon_introspection_count (assuming it
 * is zero first if it does not exist).
 *
 * While weapon_introspection_count is less than numWeapons,
 * each weapon is examined in order. After that, which
 * weapon is examined is chosen at random.
 *
 * After the run, the global ship_has_weapons is set
 * to true if ANY weapon is believed to exist, and
 * false otherwise.
 *
 * @section gvars Global Variables
 * <table>
 * <tr>
 *   <td>weapon_introspection_count</td>
 *   <td>int</td>
 *   <td>Used internally</td>
 * </tr>
 * <tr>
 *   <td>ship_has_weapons</td>
 *   <td>bool</td>
 *   <td>Indicates whether the ship has any weapons</td>
 * </tr>
 * </table>
 */
class AIM_IntroWeapon: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_IntroWeapon(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_INTRO_WEAPON_HXX_ */
