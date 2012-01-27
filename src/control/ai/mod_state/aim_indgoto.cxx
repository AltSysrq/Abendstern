/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_state/aim_indgoto.hxx
 */

/*
 * aim_indgoto.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <string>
#include <exception>
#include <stdexcept>

#include "aim_indgoto.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

using namespace std;

AIM_IndGoto::AIM_IndGoto(AIControl* c, const libconfig::Setting& s)
: AIModule(c)
{
  string name = (const char*)s["pointer"];
  if (s.exists("save_on_activation"))
    saveOnActivation=s["save_on_activation"];
  else
    saveOnActivation=false;

  if (name.size() < 2)
    throw runtime_error("pointer parm to state/igoto too short");

  if (name[0] == 'g')
    isVarGlobal=true;
  else if (name[0] == 's')
    isVarGlobal=false;
  else
    throw runtime_error("pointer parm to state/igoto begins with invalid char");

  varName = name.c_str()+1;
}

void AIM_IndGoto::activate() {
  if (saveOnActivation)
    stateName = (isVarGlobal? controller.gglob(varName.c_str(), "")
                            : controller.gstat(varName.c_str(), ""));
}

void AIM_IndGoto::action() {
  if (!saveOnActivation)
    stateName = (isVarGlobal? controller.gglob(varName.c_str(), "")
                            : controller.gstat(varName.c_str(), ""));
  controller.setState(stateName.c_str());
}

static AIModuleRegistrar<AIM_IndGoto> registrar("state/igoto");
