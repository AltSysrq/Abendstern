/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_Varset AI module
 */

/*
 * aim_varset.hxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#ifndef AIM_VARSET_HXX_
#define AIM_VARSET_HXX_

#include <libconfig.h++>
#include <utility>
#include <vector>
#include <string>

#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

/** core/set: Allows setting variables directly.
 *
 * @section config Configuration
 * &nbsp;  <code>[s/g][name] = [value];</code> <br>
 * Sets the specified name to the given scalar value. s indicates that the
 * variable is a state variable, g indicates global.
 *
 * For example,<br>
 * &nbsp;  <code>gthrust_angle = 0.1;</code> <br>
 * will set the global "thrust_angle" to 0.1.
 */
class AIM_Varset: public AIModule {
  std::vector<pair<std::string, AIControl::Variable> > vars;

  public:
  /** AIM Standard Constructor */
  AIM_Varset(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_VARSET_HXX_ */
