/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MiniGravwaveDrive system
 */

/*
 * mini_gravwave_drive.hxx
 *
 *  Created on: 21.09.2010
 *      Author: jason
 */

#ifndef MINI_GRAVWAVE_DRIVE_HXX_
#define MINI_GRAVWAVE_DRIVE_HXX_

#include "src/ship/sys/engine.hxx"

/** The MiniGravwaveDrive is the only stealth-compatible
 * engine.
 *
 * It is a miniature of the full gravity-wave
 * capital ship propulsion assembly. It is therefore very
 * low power, but has reasonable efficency (equivalent to
 * the ParticleAccelerator with one intake).
 * It requires no intakes.
 */
class MiniGravwaveDrive: public Engine {
  public:
  MiniGravwaveDrive(Ship*);

  virtual unsigned mass() const noth;
  virtual float thrust() const noth;

  virtual void audio_register() const noth;

  DEFAULT_CLONEWO(MiniGravwaveDrive)
};

#endif /* MINI_GRAVWAVE_DRIVE_HXX_ */
