/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MiniGravwaveDriveMKII system
 */

/*
 * mini_gravwave_drive_mkii.hxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#ifndef MINI_GRAVWAVE_DRIVE_MKII_HXX_
#define MINI_GRAVWAVE_DRIVE_MKII_HXX_

#include "src/ship/sys/c/mini_gravwave_drive.hxx"

/** The MiniGravwaveDriveMKII is a simple class-B improvement of the MiniGravwaveDrive.
 *
 * It multiplies power usage and thrust by two, and divides the rotational multiplier
 * by same, after running the base-class constructor.
 */
class MiniGravwaveDriveMKII: public MiniGravwaveDrive {
  public:
  MiniGravwaveDriveMKII(Ship*);
  virtual void audio_register() const noth;
  DEFAULT_CLONEWO(MiniGravwaveDriveMKII);
};

#endif /* MINI_GRAVWAVE_DRIVE_MKII_HXX_ */
