/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_misc/aim_self_destruct.hxx
 */

/*
 * aim_self_destruct.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>

#include "aim_self_destruct.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/sys/ship_system.hxx"

AIM_SelfDestruct::AIM_SelfDestruct(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_SelfDestruct::action() {
  selfDestruct(ship);
  controller.sglob("has_self_destructed", true);
}

static AIModuleRegistrar<AIM_SelfDestruct> registrar("misc/self_destruct");
