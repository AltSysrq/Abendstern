/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_FireWeapon AI module.
 */

#ifndef AIM_FIRE_WEAPON_HXX_
#define AIM_FIRE_WEAPON_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/**
 * attack/fire: Fires the current weapon if the shot quality is above a certain
 * threshhold and if !no_appropriate_weapon. It sets itself as a
 * 100 ms timer to autorepeat. It automatically disables the timer
 * if the current weapon is changed.
 *
 * @section config Configuration
 * <table>
 * <tr>
 *   <td>thresh</td>
 *   <td>float</td>
 *   <td>Aiming quality threshhold for actual firing. Defaults to 0.9.</td>
 * </tr>
 * </table>
 *
 * @section svars State Variables
 * <table>
 * <tr>
 *   <td>no_appropriate_weapon</td>
 *   <td>bool</td>
 *   <td>If set to true, will not attempt to fire a weapon.
 *       Defaults to false.</td>
 * </tr>
 * <tr>
 *   <td>shot_quality</td>
 *   <td>float</td>
 *   <td>The quality of the shot, judged against the thresh configuration.
 *       Defaults to 0.0f.</td>
 * </tr>
 * <tr>
 *   <td>veto_fire</td>
 *   <td>bool</td>
 *   <td>If true, no shots will be fired. Defaults to false.</td>
 * </table>
 */
class AIM_FireWeapon: public AIModule {
  int expectedWeapon;
  float thresh;

  public:
  /** AIM standard constructor */
  AIM_FireWeapon(AIControl*, const libconfig::Setting&);
  virtual void action();
  virtual void timer(int);
};

#endif /* AIM_FIRE_WEAPON_HXX_ */
