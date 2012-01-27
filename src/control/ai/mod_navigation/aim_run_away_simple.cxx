/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_navigation/aim_run_away_simple.hxx
 */

/*
 * aim_run_away_simple.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include "aim_run_away_simple.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/ship/ship.hxx"

AIM_RunAwaySimple::AIM_RunAwaySimple(AIControl* a, const libconfig::Setting&)
: AIModule(a)
{ }

void AIM_RunAwaySimple::action() {
  ship->configureEngines(true, false, controller.gglob("max_throttle", 1.0f));
}

static AIModuleRegistrar<AIM_RunAwaySimple> registrar("navigation/run_away_simple");
