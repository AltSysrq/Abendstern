/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/particle_accelerator.hxx
 */

#include <cmath>
#include <cstdlib>
#include <typeinfo>

#include "particle_accelerator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/background/explosion.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"
#include "src/core/lxn.hxx"

using namespace std;

#define POWER_USAGE 25
#define MAX_THRUST (300.0f/1000.0f/1000.0f)
#define EXP_SIZE 0.1f
#define EXP_LIFE 500
#define EXP_SPEED 0.00075f
#define EXP_DENSITY 0.15f/1000.0f
#define ROTMUL 0.21f

ParticleAccelerator::ParticleAccelerator(Ship* parent, GLuint tex)
: Engine(parent, tex, ROTMUL)
{
  powerUsageAmt=POWER_USAGE;
  expSize=EXP_SIZE;
  expLife=EXP_LIFE;
  expSpeed=EXP_SPEED;
  expDensity=EXP_DENSITY;
  maxThrust=MAX_THRUST;
  rm=0.1f; rb=0.9f;
  gm=0.2f; gb=0.8f;
  bm=0.0f; bb=0.3f;
  SL10N(mountingCondition,engine_mounting_conditions,particle_accelerator)
}

ParticleAccelerator::ParticleAccelerator(Ship* parent)
: Engine(parent, system_texture::particleAccelerator, ROTMUL)
{
  powerUsageAmt=POWER_USAGE;
  expSize=EXP_SIZE;
  expLife=EXP_LIFE;
  expSpeed=EXP_SPEED;
  expDensity=EXP_DENSITY;
  maxThrust=MAX_THRUST;
  rm=0.1f; rb=0.9f;
  gm=0.2f; gb=0.8f;
  bm=0.0f; bb=0.3f;
  SL10N(mountingCondition,engine_mounting_conditions,particle_accelerator)
}

unsigned ParticleAccelerator::mass() const noth {
  return 15;
}

const char* ParticleAccelerator::autoOrient() noth {
  numIntakes=0;
  return Engine::autoOrient();
}

const char* ParticleAccelerator::setOrientation(int i) noth {
  numIntakes=0;
  return Engine::setOrientation(i);
}

bool ParticleAccelerator::acceptCellFace(float angle) noth {
  if (fabs(angle)<pi/2+0.001f) {
    numIntakes += cos(angle/2);
    return true;
  }
  return false;
}

float ParticleAccelerator::thrust() const noth {
  return (parent->isStealth()? 0 : MAX_THRUST*numIntakes);
}

void ParticleAccelerator::audio_register() const noth {
  audio::particleAccelerator.addSource(container);
}
