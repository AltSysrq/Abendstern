/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_SelfDestruct AI module
 */

/*
 * aim_self_destruct.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_SELF_DESTRUCT_HXX_
#define AIM_SELF_DESTRUCT_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** misc/self_destruct: Calls self_destruct(), and sets has_self_destructed to true.
 *
 * @section gvars Global Variables
 * <table>
 * <tr><td>has_self_destructed</td><td>bool</td><td>Set to true</td></tr>
 * </table>
 */
class AIM_SelfDestruct: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_SelfDestruct(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_SELF_DESTRUCT_HXX_ */
