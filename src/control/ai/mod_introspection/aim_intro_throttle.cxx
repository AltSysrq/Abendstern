/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_introspection/aim_intro_throttle.hxx
 */

/*
 * aim_intro_throttle.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <cstdlib>

#include "aim_intro_throttle.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

using namespace std;

AIM_IntroThrottle::AIM_IntroThrottle(AIControl* c, const libconfig::Setting&)
: AIModule(c)
{ }

void AIM_IntroThrottle::action() {
  //Begin by backing the current settings up
  bool wasAccelOn = ship->isThrustOn();
  bool wasBrakeOn = ship->isBrakeOn();
  float oldThrustLevel = ship->getThrust();

  float newLevel = rand() / (float)RAND_MAX;
  ship->configureEngines(true, false, newLevel);
  bool accelOk = ship->getPowerUsagePercent() <= 1;
  ship->configureEngines(false, true, newLevel);
  bool brakeOk = ship->getPowerUsagePercent() <= 1;

  //Restore
  ship->configureEngines(wasAccelOn, wasBrakeOn, oldThrustLevel);

  float currMaxAccel = controller.gglob("max_throttle", newLevel);
  float currMaxBrake = controller.gglob("max_brake", newLevel);
  if ((currMaxAccel > newLevel && !accelOk)
  ||  (currMaxAccel < newLevel &&  accelOk))
    controller.sglob("max_throttle", newLevel);
  if ((currMaxBrake > newLevel && !brakeOk)
  ||  (currMaxBrake < newLevel &&  brakeOk))
    controller.sglob("max_brake", newLevel);
}

static AIModuleRegistrar<AIM_IntroThrottle> registrar("introspection/throttle");
