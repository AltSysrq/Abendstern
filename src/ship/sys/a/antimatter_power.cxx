/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/antimatter_power.hxx
 */

#include <GL/gl.h>
#include <cmath>

#include "antimatter_power.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/sim/blast.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/weapon/particle_beam.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

AntimatterPower::AntimatterPower(Ship* s)
: PowerPlant(s, system_texture::antimatterPower)
{ }

signed AntimatterPower::normalPowerUse() const noth {
  return -150;
}

unsigned AntimatterPower::mass() const noth {
  return 150;
}

void AntimatterPower::destroy(unsigned blame) noth {
  pair<float,float> coords=Ship::cellCoord(container->parent, container, 0, 0);
  GameField* field=container->parent->getField();
  //Don't create a Blast if remote
  if (!parent->isRemote)
    field->inject(new Blast(field, blame, coords.first, coords.second, STD_CELL_SZ*2.5f,
                            (container->parent->hasPower()? 100 : 50), true, STD_CELL_SZ/2));
  if (!EXPCLOSE(coords.first, coords.second)) return;
  pair<float,float> vel=container->parent->getCellVelocity(container);
  if (container->parent->hasPower()) {
    bool incursion = !(rand()&0x3F);
    Explosion ex(field, incursion? Explosion::Incursion : Explosion::BigSpark,
                 0.75f, 0.75f, 1.0f, 3.4, 0.06f, 5000, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  } else {
    Explosion ex(field, Explosion::BigSpark,
                 0.3f, 0.3f, 0.6f, 0.5f, 0.0075f, 3500, coords.first, coords.second, vel.first, vel.second);
    ex.hungry=true;
    field->add(&ex);
  }
}

bool AntimatterPower::particleBeamCollision(const ParticleBurst* other) noth {
  if (other->type == AntiprotonBeam) {
    destroy(other->blame);
    return false;
  }
  return true;
}

void AntimatterPower::audio_register() const noth {
  audio::antimatterPower.addSource(container);
}
