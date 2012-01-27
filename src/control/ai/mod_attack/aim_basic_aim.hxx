/**
 * @file
 * @author Jason Lingle
 * @brief Contains class AIM_BasicAim AI module
 */

#ifndef AIM_BASIC_AIM_HXX_
#define AIM_BASIC_AIM_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/**
 * attack/basic_aim: Aim at the target, leading it as necessary.
 *
 * Set throttle according to normalised dot-
 * product between target facing us and
 * the vector between the target and us.
 * @section config Configuration
 * <table>
 * <tr>
 *  <td>attack_speed</td>           <td>float</td>
 *    <td>Indicates appropriate speed for attacking. Defaults to 0.0005f.</td>
 * </tr>
 * </table>
 *
 * @section svars State Variables
 * <table>
 * <tr>
 *  <td>suggest_target_offset_x</td><td>float</td>   <td>Offset to add to target X. Defaults to 0.0f.</td>
 * </tr><tr>
 *  <td>suggest_target_offset_y</td><td>float</td>   <td>Offset to add to target Y. Defaults to 0.0f.</td>
 * </tr><tr>
 *  <td>suggest_angle_offset</td>   <td>float</td>   <td>Offset to add to angle to target. Defaults to 0.0f</td>
 * </tr><tr>
 *  <td>shot_quality</td>           <td>float</td>   <td>
 *        1.0 indicates perfect shot; anything less than 0 indicates
 *        no chance of hitting </td>
 * </tr>
 * </table>
 */
class AIM_BasicAim: public AIModule {
  float speed;
  public:
  /** AIM standard constructor. */
  AIM_BasicAim(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_BASIC_AIM_HXX_ */
