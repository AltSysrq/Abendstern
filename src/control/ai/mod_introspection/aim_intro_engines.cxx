/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_introspection/aim_intro_engines.hxx
 */

/*
 * aim_intro_engines.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include "aim_intro_engines.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

AIM_IntroEngines::AIM_IntroEngines(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_IntroEngines::action() {
  bool wasThrustOn = ship->isThrustOn();
  bool wasBrakeOn = ship->isBrakeOn();
  float throttle = ship->getThrust();
  ship->configureEngines(true, false, 1);
  controller.sglob("thrust_angle", ship->getThrustAngle());
  ship->configureEngines(wasThrustOn, wasBrakeOn, throttle);
}

static AIModuleRegistrar<AIM_IntroEngines> registrar("introspection/engines");
