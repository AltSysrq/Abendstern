/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ParticleAccelerator system
 */

#ifndef PARTICLE_ACCELERATOR_HXX_
#define PARTICLE_ACCELERATOR_HXX_
#include <utility>

#include "src/ship/sys/engine.hxx"

/** A ParticleAccelerator is a simple engine that
 * draws in particles from the surrounding space
 * and accelerates them rapidly backwards.
 */
class ParticleAccelerator: public Engine {
  friend class TclParticleAccelerator;
  protected:
  /** The more side and front slots are open, the
   * greater our thrust. An intake's input value
   * is cos(offset/2), where offset is the
   * angle between 12 o'clock and the side.
   */
  float numIntakes;
  /** The colour of the system (a float[3]). */
  float* colour;

  /** Constructs a ParticleAccelerator with a non-default system texture. */
  ParticleAccelerator(Ship* parent, GLuint tex);

  public:
  ParticleAccelerator(Ship* parent);

  //These two do not define new behaviour, but just
  //set numIntakes to 0, then call the Engine:: one.
  virtual const char* autoOrient() noth;
  virtual const char* setOrientation(int) noth;

  virtual bool acceptCellFace(float) noth;
  virtual unsigned mass() const noth;
  virtual float thrust() const noth;

  virtual void audio_register() const noth;

  DEFAULT_CLONEWO(ParticleAccelerator)
};

#endif /*PARTICLE_ACCELERATOR_HXX_*/
