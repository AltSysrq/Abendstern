/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_IntroEngines AI module
 */

/*
 * aim_intro_engines.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_INTRO_ENGINES_HXX_
#define AIM_INTRO_ENGINES_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** introspection/engines: Determine properties about the engines, namely which
 * direction they propel the craft.
 *
 * @section gvars Global Variables
 * <table><tr>
 *   <td>thrust_angle</td>
 *   <td>float</td>
 *   <td>The angle the engines propel the ship.</td>
 * </tr></table>
 */
class AIM_IntroEngines: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_IntroEngines(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_INTRO_ENGINES_HXX_ */
