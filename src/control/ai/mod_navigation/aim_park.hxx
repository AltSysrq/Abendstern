/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_Park AI module
 */

/*
 * aim_park.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_PARK_HXX_
#define AIM_PARK_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** navigation/park: Set brake, kill accel, maximise throttle.
 *
 * @section gvars Global Variables
 * <table>
 * <tr><td>max_brake</td><td>float</td><td>Brake amount to use; defaults to 1.0f.</td></tr>
 * </table>
 */
class AIM_Park: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_Park(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_PARK_HXX_ */
