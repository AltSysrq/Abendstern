/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Contains the AIM_VetoFire AI module
 */
#ifndef AIM_VETO_FIRE_HXX_
#define AIM_VETO_FIRE_HXX_

#include <libconfig.h++>

#include "src/control/ai/aimod.hxx"

/**
 * attack/veto_fire: perform basic checks to see whether using a weapon is OK
 *
 * When executed, scans ahead of the ship at random points with distance
 * between 0 and the distance to the target, along the trajectory of the
 * weapon being launched (assuming it moves in a straight line). If the weapon
 * would collide with any ship within our radius, or it would hit a friend,
 * veto the use of the weapon.
 *
 * @section config Configuration
 * <table>
 * <tr>
 *   <td>trials</td>
 *   <td>int</td>
 *   <td>Number of points to test; defaults to 64.</td>
 * </tr>
 * </table>
 *
 * @section svars State Variables
 * <table>
 * <tr>
 *   <td>no_appropriate_weapon</td>
 *   <td>bool</td>
 *   <td>If set to true, action is aborted. Defaults to false.</td>
 * </tr>
 * <tr>
 *   <td>suggest_angle_offset</td>
 *   <td>float</td>
 *   <td>
 *     Set to what this module considers the best angle offset
 *     for the selected weapon.
 *   </td>
 * </tr>
 * <tr>
 *   <td>veto_fire</td>
 *   <td>bool</td>
 *   <td>Set to true if this module thinks that using the weapon is a bad idea.
 *   </td>
 * </tr>
 * </table>
 */
class AIM_VetoFire: public AIModule {
  const unsigned trials;

public:
  ///AIM standard constructor
  AIM_VetoFire(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_VETO_FIRE_HXX_ */
