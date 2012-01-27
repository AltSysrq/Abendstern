/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_NavToTarget AI module
 */

/*
 * aim_nav_to_target.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_NAV_TO_TARGET_HXX_
#define AIM_NAV_TO_TARGET_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** navigation/target: Simple algorithm to navigate to the current target.
 *
 * Does nothing if no target.
 *
 * @section config Configuration
 * <table><tr>
 *   <td>cruising_speed</td><td>float</td><td>Max speed. Defaults to 2.5f.</td>
 * </tr></table>
 *
 * @section gvars Global Variables
 * <table>
 *   <tr><td>max_throttle</td><td>float</td><td>Maximum throttle to use; defaults to 1.0f.</td></tr>
 *   <tr><td>max_brake</td>   <td>float</td><td>Maximum brake to use; defaults to 1.0f.</td></tr>
 *   <tr><td>thrust_angle</td><td>float</td><td>Expected direction of propulsion; defaults to 0.0f.</td></tr>
 * </table>
 */
class AIM_NavToTarget: public AIModule {
  float cruisingSpeed;

  public:
  /** AIM Standard Constructor */
  AIM_NavToTarget(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_NAV_TO_TARGET_HXX_ */
