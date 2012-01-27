/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SuperParticleAccelerator system
 */

#ifndef SUPER_PARTICLE_ACCELERATOR_HXX_
#define SUPER_PARTICLE_ACCELERATOR_HXX_

#include "src/ship/sys/c/particle_accelerator.hxx"

/** The SuperParticleAccelerator is simply a more
 * powerful Class-B extension of the Class-C
 * ParticleAccelerator.
 *
 * It is far less efficient
 * than a BussardRamjet, but does not require
 * an exposed front face.
 */
class SuperParticleAccelerator: public ParticleAccelerator {
  public:
  SuperParticleAccelerator(Ship*);
  virtual float thrust() const noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONEWO(SuperParticleAccelerator)
};

#endif /*SUPER_PARTICLE_ACCELERATOR_HXX_*/
