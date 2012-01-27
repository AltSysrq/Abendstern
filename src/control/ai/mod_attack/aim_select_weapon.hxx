/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_SelectWeapon AI module.
 */

#ifndef AIM_SELECT_WEAPON_HXX_
#define AIM_SELECT_WEAPON_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** attack/select_weapon: Uses basic introspective information and information on the
 * target to select a weapon.
 *
 * Does nothing if there is no target.
 *
 * @section conf Configuration
 * <table>
 * <tr>
 *   <td>temperature_caution</td>
 *   <td>float</td>
 *   <td>Avoid using plasma weapons when the temperature percent exceeds this. Defaults to 0.95</td>
 * </tr>
 * <tr>
 *   <td>min_monophase_size</td>
 *   <td>float</td>
 *   <td>Avoid using the MonophasicEnergyPulse on ships smaller than this,
 *       in screen coordinates. Defaults to 0.2f.</td>
 * </tr>
 * <tr>
 *   <td>magneto_caution</td>
 *   <td>float</td>
 *   <td> The mimimum value of the following equation required for us to
 *     consider using magneto bombs:
 *       (target mass)/(our mass)/distance
 *     Defaults to 2.0f.</td>
 * </tr>
 * </table>
 *
 * @section svars State Variables
 * <table>
 * <tr>
 *   <td>no_appropriate_weapon</td>
 *   <td>bool</td>
 *   <td>
 *     Set to true if the module did not find a good weapon for
 *     the current situation; set to false otherwise.
 *   </td>
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
 *   <td>suggest_target_offset_x</td>
 *   <td>float</td>
 *   <td>See suggest_target_offset_y. Defaults to 0.0f.</td>
 * </tr>
 * <tr>
 *   <td>suggest_target_offset_y</td>
 *   <td>float</td>
 *   <td>
 *     Add these values to the position of the target when computing
 *     angles et cetera. This should be provided by the aiming
 *     module so that this module can understand how it is trying
 *     to lead the target.
 *     Defaults to 0.0f.</td>
 * </tr>
 * </table>
 */
class AIM_SelectWeapon: public AIModule {
  float temperatureCaution, minMonophaseSize, magnetoCaution;

  public:
  /** AIM Standard Constructor */
  AIM_SelectWeapon(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_SELECT_WEAPON_HXX_ */
