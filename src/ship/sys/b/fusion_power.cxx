/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/fusion_power.hxx
 */

#include <GL/gl.h>
#include <cmath>
#include <cstdlib>

#include "fusion_power.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/blast.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/weapon/particle_beam.hxx"
#include "src/audio/ship_effects.hxx"
using namespace std;

FusionPower::FusionPower(Ship* s)
: PowerPlant(s, system_texture::fusionPower)
{ }

signed FusionPower::normalPowerUse() const noth {
  return -50;
}

unsigned FusionPower::mass() const noth {
  return 100;
}

void FusionPower::destroy(unsigned blame) noth {
  pair<float,float> coords=Ship::cellCoord(parent, container);
  GameField* field=parent->getField();
  //Don't create a Blast if remote
  if (!parent->isRemote) {
    Blast* blast = new Blast(field, blame, coords.first, coords.second,
                             STD_CELL_SZ*1.5f, (parent->hasPower()? 35 : 15),
                             true, STD_CELL_SZ/2);
    parent->injectBlastCollision(blast);
    field->inject(blast);
  }

  if (!EXPCLOSE(coords.first, coords.second)) return;
  pair<float,float> vel=parent->getCellVelocity(container);
  if (parent->hasPower()) {
    bool incursion = !(rand()&0x3F);
    Explosion ex(field, incursion? Explosion::Incursion : Explosion::BigSpark,
                 1.0f, 0.5f, 0.1f, 3, 0.05f, 5000, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  } else {
    Explosion ex(field, Explosion::BigSpark,
                 0.8f, 0.3f, 0.1f, 0.4f, 0.0075f, 3500, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  }
}

bool FusionPower::particleBeamCollision(const ParticleBurst* other) noth {
  if (other->type == MuonBeam) {
    destroy(other->blame);
    return false;
  }
  return true;
}

void FusionPower::audio_register() const noth {
  audio::fusionPower.addSource(container);
}
