/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_state/aim_goto.hxx
 */

/*
 * aim_goto.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <string>

#include "aim_goto.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

AIM_Goto::AIM_Goto(AIControl* c, const libconfig::Setting& s)
: AIModule(c), stateName((const char*)s["target"])
{ }

void AIM_Goto::action() {
  controller.setState(stateName.c_str());
}

static AIModuleRegistrar<AIM_Goto> registrar("state/goto");
