/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_IntroThrottle AI module
 */

/*
 * aim_intro_throttle.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_INTRO_THROTTLE_HXX_
#define AIM_INTRO_THROTTLE_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** introspection/throttle: Determines the current throttle capabilities of the ship.
 *
 * Picks a random throttle level and checks to see if it works
 * with accel and (exclusive) brake. If it does, and the respective
 * global variable is lower, it is increased to the new value. If it
 * does not, but the global variable is greater, it is decreased to
 * the new value.
 *
 * @section gvars Global Variables
 * <table>
 * <tr>
 *   <td>max_throttle</td>
 *   <td>float</td>
 *   <td>Maximum throttle that does not exceed power production</td>
 * </tr><tr>
 *   <td>max_brake</td>
 *   <td>float</td>
 *   <td>Maximum braking throttle that does not exceed power production</td>
 * </tr>
 * </table>
 */
class AIM_IntroThrottle: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_IntroThrottle(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_INTRO_THROTTLE_HXX_ */
