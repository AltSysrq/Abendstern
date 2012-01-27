/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_Goto AI module
 */

/*
 * aim_goto.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_GOTO_HXX_
#define AIM_GOTO_HXX_

#include <libconfig.h++>
#include <string>

#include "src/control/ai/aimod.hxx"

/** state/goto: Immediately and unconditionally changes to the specified state.
 *
 * @section config Config
 * <table>
 * <tr><td>target</td><td>string</td><td>Name of target state</td></tr>
 * </table>
 */
class AIM_Goto: public AIModule {
  std::string stateName;

  public:
  /** AIM Standard Constructor */
  AIM_Goto(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_GOTO_HXX_ */
