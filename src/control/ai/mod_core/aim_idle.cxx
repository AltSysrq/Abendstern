/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_core/aim_idle.hxx
 */

/*
 * aim_idle.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include "aim_idle.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

AIM_Idle::AIM_Idle(AIControl* aic, const libconfig::Setting&)
: AIModule(aic)
{ }

static AIModuleRegistrar<AIM_Idle> registrar("core/idle");
