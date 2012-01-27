/**
 * @file
 * @author Jason Lingle
 * @brief  This file defines locations of the texture names
 * for all normal systems.
 */

/*
 * system_textures.hxx
 *
 *  Created on: 26.01.2011
 *      Author: jason
 */

#ifndef SYSTEM_TEXTURES_HXX_
#define SYSTEM_TEXTURES_HXX_

#include <GL/gl.h>

/** Contains declarations for all system textures.
 * All these should be treated
 * as if they were const, even though they are not.
 * The system_texture::load() function must be called before any values
 * here are valid.
 * @see system_texture::load()
 */
namespace system_texture {
  extern /* const */ GLuint
  shieldGenerator,
  dispersionShield,
  capacitor,
  fissionPower,
  fusionPower,
  powerCell,
  antimatterPower,
  heatsink,
  particleAccelerator,
  superParticleAccelerator,
  bussardRamjet,
  miniGravwaveDrive,
  relIonAccelerator,
  energyChargeLauncher,
  magnetoBombLauncher,
  semiguidedBombLauncher,
  plasmaBurstLauncher,
  gatlingPlasmaBurstLauncher,
  monophasicEnergyEmitter,
  missileLauncher,
  particleBeamLauncher,
  reinforcementBulkhead,
  selfDestructCharge,
  cloakingDevice,
  squareBridge,
  equtBridge;

  /** Loads all system textures.
   *
   * Aborts program on failure.
   */
  void load();
}

#endif /* SYSTEM_TEXTURES_HXX_ */
