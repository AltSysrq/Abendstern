/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/mini_gravwave_drive.hxx
 */

/*
 * mini_gravwave_drive.cxx
 *
 *  Created on: 21.09.2010
 *      Author: jason
 */
#include <GL/gl.h>

#include "mini_gravwave_drive.hxx"
#include "src/ship/sys/engine.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"
#include "src/core/lxn.hxx"

using namespace std;

#define POWER_USAGE 20
#define MAX_THRUST (240.0f/1000.0f/1000.0f)
#define EXP_SIZE -1


MiniGravwaveDrive::MiniGravwaveDrive(Ship* s)
: Engine(s, system_texture::miniGravwaveDrive, 0.5f) {
  powerUsageAmt=POWER_USAGE;
  expSize=EXP_SIZE;
  maxThrust=MAX_THRUST;
  _supportsStealthMode=true;

  SL10N(mountingCondition,engine_mounting_conditions,mini_gravwave_drive)
}

unsigned MiniGravwaveDrive::mass() const noth {
  return 200;
}

float MiniGravwaveDrive::thrust() const noth {
  return maxThrust;
}

void MiniGravwaveDrive::audio_register() const noth {
  audio::miniGravwaveDrive.addSource(container);
}
