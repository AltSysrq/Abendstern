/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/fission_power.hxx
 */

#include <GL/gl.h>
#include <cmath>
#include <cstdlib>

#include "fission_power.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/blast.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/weapon/particle_beam.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

FissionPower::FissionPower(Ship* s)
: PowerPlant(s, system_texture::fissionPower)
{ }

signed FissionPower::normalPowerUse() const noth {
  return -25;
}

unsigned FissionPower::mass() const noth {
  return 75;
}

void FissionPower::destroy(unsigned blame) noth {
  pair<float,float> coords=Ship::cellCoord(parent, container);
  GameField* field=container->parent->getField();
  //Don't create blast if remote
  if (!parent->isRemote)
    field->inject(new Blast(field, blame, coords.first, coords.second, STD_CELL_SZ*1.5f,
                            (parent->hasPower()? 15 : 5), true, STD_CELL_SZ/2));
  if (!EXPCLOSE(coords.first, coords.second)) return;
  pair<float,float> vel=parent->getCellVelocity(container);
  if (parent->hasPower()) {
    bool incursion = !(rand()&0x3F);
    Explosion ex(field, (!incursion? Explosion::BigSpark : Explosion::Incursion),
                 0.2f, 1.0f, 0.1f, 3, 0.05f, 5000, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  } else {
    Explosion ex(field, Explosion::BigSpark,
                 0.3f, 0.8f, 0.1f, 0.4f, 0.0075f, 3500, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  }
}

bool FissionPower::particleBeamCollision(const ParticleBurst* other) noth {
  if (other->type == NeutronBeam) {
    destroy(other->blame);
    return false;
  }
  return true;
}

void FissionPower::audio_register() const noth {
  audio::fissionPower.addSource(container);
}
