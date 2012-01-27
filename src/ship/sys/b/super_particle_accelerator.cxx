/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/super_particle_accelerator.hxx
 */

#include "super_particle_accelerator.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"

#define POWER_USAGE 50
#define MAX_THRUST (600.0f/1000.0f/1000.0f)
#define EXP_SIZE 0.2f

SuperParticleAccelerator::SuperParticleAccelerator(Ship* s)
: ParticleAccelerator(s, system_texture::superParticleAccelerator)
{
  powerUsageAmt=POWER_USAGE;
  maxThrust=MAX_THRUST;
  expSize=EXP_SIZE;
}

float SuperParticleAccelerator::thrust() const noth {
  return MAX_THRUST*numIntakes;
}

void SuperParticleAccelerator::audio_register() const noth {
  audio::superParticleAccelerator.addSource(container);
}
