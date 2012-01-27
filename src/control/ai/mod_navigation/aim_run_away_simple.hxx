/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_RunAwaySimple AI module
 */

/*
 * aim_run_away_simple.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_RUN_AWAY_SIMPLE_HXX_
#define AIM_RUN_AWAY_SIMPLE_HXX_

#include <libconfig.h++>

#include "src/control/ai/aimod.hxx"

/** navigation/run_away_simple: Set throttle to full and exit the field.
 *
 * @section gvars Global Variables
 * <table>
 * <tr><td>max_throttle</td><td>float</td><td>Maximum throttle that can be used; defaults to 1.0f.</td></tr>
 * </table>
 */
class AIM_RunAwaySimple: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_RunAwaySimple(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_RUN_AWAY_SIMPLE_HXX_ */
