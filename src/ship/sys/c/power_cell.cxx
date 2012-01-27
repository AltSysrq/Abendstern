/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/power_cell.hxx
 */

#include "power_cell.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/weapon/particle_beam.hxx"

PowerCell::PowerCell(Ship* parent)
: PowerPlant(parent, system_texture::powerCell) {
  _supportsStealthMode=true;
}

int PowerCell::normalPowerUse() const noth {
  return -15;
}

unsigned PowerCell::mass() const noth {
  return 50;
}

bool PowerCell::particleBeamCollision(const ParticleBurst* other) noth {
  return other->type != PositronBeam;
}
