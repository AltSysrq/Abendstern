/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/rel_ion_accelerator.hxx
 */

#include "rel_ion_accelerator.hxx"
#include "src/ship/sys/engine.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

#define POWER_USAGE 200
#define MAX_THRUST (2000.0f/1000.0f/1000.0f)
#define EXP_SIZE 0.1f
#define EXP_LIFE 350
#define EXP_SPEED 0.0020f
#define EXP_DENSITY 0.075f/1000.0f
#define ROTMUL 0.0f

RelIonAccelerator::RelIonAccelerator(Ship* s)
: Engine(s, system_texture::relIonAccelerator, ROTMUL)
{
  powerUsageAmt=POWER_USAGE;
  expSize=EXP_SIZE;
  expLife=EXP_LIFE;
  expSpeed=EXP_SPEED;
  expDensity=EXP_DENSITY;
  maxThrust=MAX_THRUST;
  //Primarily light blue,, but other colours mixed in
  rm=0.5f; rb=0.6f;
  gm=0.5f; gb=0.6f;
  bm=0.1f; bb=0.9f;

  SL10N(mountingCondition,engine_mounting_conditions,rel_ion_accelerator)
}

unsigned RelIonAccelerator::mass() const noth {
  return 100;
}

float RelIonAccelerator::thrust() const noth {
  return MAX_THRUST;
}

void RelIonAccelerator::audio_register() const noth {
  audio::relIonAccelerator.addSource(container);
}
