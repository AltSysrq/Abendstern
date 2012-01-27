/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_Procedure AI module
 */

/*
 * aim_procedure.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_PROCEDURE_HXX_
#define AIM_PROCEDURE_HXX_

#include <libconfig.h++>

#include "src/control/ai/aimod.hxx"

/** core/procedure: Adds a list of modules to the current procedure list.
 *
 * Modules in a procedure do not normally get activation/deactivation
 * messages. If a module reports that it requires such notifications,
 * it is automatically wrapped within another internal module whose
 * action is to activate, action, and deactivate the child module.

 * @section config Configuration
 * The format is <br>
 * &nbsp;  <code>modules = ( ... )</code><br>
 * The modules list has the same format as for the normal AI.
 * The weight parameters for each module are optional, and default
 * to 1 if not specified. The weight parameter determines how many
 * times the same module is added to the list.
 */
class AIM_Procedure: public AIModule {
  struct Child { AIModule* module; unsigned cnt; } * children;
  unsigned numChildren;

  public:
  /** AIM Standard Constructor */
  AIM_Procedure(AIControl*, const libconfig::Setting&);
  virtual ~AIM_Procedure();

  virtual void action();
};

#endif /* AIM_PROCEDURE_HXX_ */
