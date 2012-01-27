/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_navigation/aim_park.hxx
 */

/*
 * aim_park.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>

#include "aim_park.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

AIM_Park::AIM_Park(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_Park::action() {
  ship->configureEngines(false, true, controller.gglob("max_brake", 1.0f));
}

static AIModuleRegistrar<AIM_Park> registrar("navigation/park");
