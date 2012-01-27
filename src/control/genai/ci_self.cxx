/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_self.hxx
 */

/*
 * ci_self.cxx
 *
 *  Created on: 29.10.2011
 *      Author: jason
 */

#include <map>
#include <string>
#include <cstring>
#include <cmath>

#include "ci_self.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"

using namespace std;

void cortex_input::SelfSource::getInputs(float* dst) noth {
  if (!ready) {
    inputs[InputOffset::x]      = ssship->getX();
    inputs[InputOffset::y]      = ssship->getY();
    inputs[InputOffset::t]      = ssship->getRotation();
    inputs[InputOffset::vx]     = ssship->getVX();
    inputs[InputOffset::vy]     = ssship->getVY();
    inputs[InputOffset::vt]     = ssship->getVRotation();
    //It's OK to destroy old parms
    ssship->configureEngines(false,false,1.0f);
    inputs[InputOffset::powerumin]      = ssship->getPowerDrain();
    ssship->configureEngines(true,false,1.0f);
    inputs[InputOffset::powerumax]      = ssship->getPowerDrain();
    inputs[InputOffset::powerprod]      = ssship->getPowerSupply();
    inputs[InputOffset::capac]          = ssship->getCurrentCapacitance();
    inputs[InputOffset::acc]            = ssship->getAcceleration();
    inputs[InputOffset::rota]           = ssship->getRotationAccel();
    inputs[InputOffset::spin]           = ssship->getUncontrolledRotationAccel();
    inputs[InputOffset::rad]            = ssship->getRadius();
    inputs[InputOffset::mass]           = ssship->getMass();

    //Normalise theta
    float theta = inputs[InputOffset::t];
    theta = fmod(theta, 2*pi);
    if (theta > pi) theta -= 2*pi;
    else if (theta <= -pi) theta += 2*pi;
    inputs[InputOffset::t] = theta;

    ready = true;
  }
  memcpy(dst, inputs, sizeof(inputs));
}
