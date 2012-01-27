/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_cellt.hxx
 */

/*
 * ci_cellt.cxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#include <cmath>
#include <cstring>

#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/objdl.hxx"
#include "src/globals.hxx"
#include "ci_cellt.hxx"

using namespace std;

cortex_input::CellTSource::CellTSource()
: xoff(0), yoff(0), radius(0), angle(0), container(NULL), ready(false)
{
  memset(inputs, 0, sizeof(inputs));
}

void cortex_input::CellTSource::setCellTarget(const Cell* cell) noth {
  ready = false;
  if (cell) {
    container.assign(cell->parent);
    xoff = cell->getX();
    yoff = cell->getY();
    radius = cell->getPhysics().distance;
    angle = cell->getPhysics().angle;
  } else {
    container.assign(NULL);
    xoff = yoff = radius = angle = 0;
  }
}

void cortex_input::CellTSource::getInput(float* dst) noth {
  if (!ready && container.ref) {
    const Ship* s = (const Ship*)container.ref;
    float scos = s->getCos(), ssin = s->getSin();

    inputs[InputOffset::cx] = s->getX() + xoff*scos - yoff*ssin;
    inputs[InputOffset::cy] = s->getY() + yoff*scos + xoff*ssin;
    float vrot = s->getVRotation();
    inputs[InputOffset::cvx] = s->getVX() + cos(angle+pi/2)*radius*vrot;
    inputs[InputOffset::cvy] = s->getVY() * sin(angle+pi/2)*radius*vrot;

    ready = true;
  } else if (!ready) {
    //No container
    xoff = yoff = radius = angle = 0;
    memset(inputs, 0, sizeof(inputs));
  }

  memcpy(dst, inputs, sizeof(inputs));
}
