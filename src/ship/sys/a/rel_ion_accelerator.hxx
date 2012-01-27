/**
 * @file
 * @author Jason Lingle
 * @brief Contains the RelIonAccelerator system
 */

#ifndef REL_ION_ACCELERATOR_HXX_
#define REL_ION_ACCELERATOR_HXX_

#include "src/ship/sys/engine.hxx"

/** The Class A engine is the Relativistic
 * Ion Accelerator.
 *
 * It provides a large amount of thrust,
 * but also consumes a very large amount of power, being
 * less efficient than even the Class C Particle Accelerator.
 * However, it requires no intakes.
 */
class RelIonAccelerator: public Engine {
  public:
  RelIonAccelerator(Ship*);

  virtual unsigned mass() const noth;
  virtual float thrust() const noth;
  virtual void audio_register() const noth;
  DEFAULT_CLONEWO(RelIonAccelerator)
};

#endif
