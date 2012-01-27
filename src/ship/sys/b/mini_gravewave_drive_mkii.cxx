/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/mini_gravwave_drive_mkii.hxx
 */

/*
 * mini_gravewave_drive_mkii.cxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#include "mini_gravwave_drive_mkii.hxx"
#include "src/audio/ship_effects.hxx"

#define MUL 2

MiniGravwaveDriveMKII::MiniGravwaveDriveMKII(Ship* s)
: MiniGravwaveDrive(s)
{
  powerUsageAmt *= MUL;
  maxThrust *= MUL;
  rotationalMultiplier /= MUL;
}

void MiniGravwaveDriveMKII::audio_register() const noth {
  audio::miniGravwaveDriveII.addSource(container);
}
