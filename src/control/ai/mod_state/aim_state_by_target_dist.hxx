/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_StateByTargetDist AI module
 */

/*
 * aim_state_by_target_dist.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_STATE_BY_TARGET_DIST_HXX_
#define AIM_STATE_BY_TARGET_DIST_HXX_

#include <libconfig.h++>
#include <string>

#include "src/control/ai/aimod.hxx"

/** state/target_distance: Changes state based on the distance of the target.
 * Does nothing if there is no target.
 *
 * @section config Configuration
 * <table>
 *   <tr>
 *     <td>consider_this_close</td>
 *     <td>float</td>
 *     <td>Threshhold distance between "close" and "far"</td>
 *   </tr><tr>
 *     <td>close_state</td><td>string</td><td>State to change to when close</td>
 *   </tr><tr>
 *     <td>far_state</td><td>string</td><td>State to change to when far</td>
 *   </tr>
 * </table>
 */
class AIM_StateByTargetDist: public AIModule {
  float considerThisClose;
  std::string closeState, farState;

  public:
  /** AIM Standard Constructor */
  AIM_StateByTargetDist(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_STATE_BY_TARGET_DIST_HXX_ */
