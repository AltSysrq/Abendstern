/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/capacitor.hxx
 */

#include <cmath>
#include <iostream>
#include <GL/gl.h>

#include "src/ship/cell/cell.hxx"
#include "capacitor.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

Capacitor::Capacitor(Ship* par, unsigned cap)
: ShipSystem(par, system_texture::capacitor), capacity(cap)
{ }

unsigned Capacitor::mass() const noth {
  //Make the mass increase exponentially
  //We still want mass=30 when capacity=30, though
  float capD30=capacity/30.0f;
  return (unsigned)(30.0f*capD30*capD30);
}

unsigned Capacitor::getCapacity() noth {
  return capacity;
}

bool Capacitor::setCapacity(unsigned c) noth {
  if (c>5 && c<100) capacity=c;
  else return false;
  return true;
}

void Capacitor::write(ostream& out) noth {
  out << capacity << ' ';
}

ShipSystem* Capacitor::clone() noth {
  return new Capacitor(parent, capacity);
}

void Capacitor::audio_register() const noth {
  for (unsigned i=0; i<lenof(audio::capacitorRing); ++i)
    if (audio::capacitorRing[i])
      audio::capacitorRing[i]->addSource(container);
}
