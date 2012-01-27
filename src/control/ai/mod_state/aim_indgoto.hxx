/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_IndGoto AI module
 */

/*
 * aim_indgoto.hxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#ifndef AIM_INDGOTO_HXX_
#define AIM_INDGOTO_HXX_

#include <libconfig.h++>
#include <string>
#include "src/control/ai/aimod.hxx"

/** state/igoto: Perform indirect goto.
 *
 * @section config Configuration
 * <table>
 *   <tr>
 *     <td>pointer</td>
 *     <td>string</td>
 *     <td>Type and name of variable to use for the state name</td>
 *   </tr><tr>
 *     <td>save_on_activation</td>
 *     <td>bool</td>
 *     <td>See explanation below; defaults to false</td>
 *   </tr>
 * </table>
 * The pointer starts with a 'g' if the rest of the name is to be used
 * for a global variable, and with an 's' if a state variablle.
 * If save_on_activation is true, the variable is copied when the module
 * is activated, instead of being looked up on action. This allows
 * a form of non-recursive call stack to be constructed.
 */
class AIM_IndGoto: public AIModule {
  std::string stateName;
  std::string varName;
  bool isVarGlobal;
  bool saveOnActivation;

  public:
  /** AIM Standard Constructor */
  AIM_IndGoto(AIControl*, const libconfig::Setting&);
  virtual void activate();
  virtual void action();
};

#endif /* AIM_INDGOTO_HXX_ */
