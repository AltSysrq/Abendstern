/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/system_textures.hxx
 */

/*
 * system_textures.cxx
 *
 *  Created on: 26.01.2011
 *      Author: jason
 */

#include <GL/gl.h>

#include "src/graphics/index_texture.hxx"
#include "system_textures.hxx"

namespace system_texture {
  GLuint
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
  monophasicEnergyEmitter,
  missileLauncher,
  particleBeamLauncher,
  gatlingPlasmaBurstLauncher,
  reinforcementBulkhead,
  selfDestructCharge,
  cloakingDevice,
  squareBridge,
  equtBridge;

  void load() {
    loadIndexTextures("images/sys/info.rc", "images/sys/",
                      "DispersionShield",       &dispersionShield,
                      "ShieldGenerator",        &shieldGenerator,
                      "Capacitor",              &capacitor,
                      "FissionPower",           &fissionPower,
                      "FusionPower",            &fusionPower,
                      "PowerCell",              &powerCell,
                      "AntimatterPower",        &antimatterPower,
                      "Heatsink",               &heatsink,
                      "ParticleAccelerator",    &particleAccelerator,
                      "SuperParticleAccelerator",&superParticleAccelerator,
                      "BussardRamjet",          &bussardRamjet,
                      "MiniGravwaveDrive",      &miniGravwaveDrive,
                      "RelIonAccelerator",      &relIonAccelerator,
                      "EnergyChargeLauncher",   &energyChargeLauncher,
                      "MagnetoBombLauncher",    &magnetoBombLauncher,
                      "SemiguidedBombLauncher", &semiguidedBombLauncher,
                      "PlasmaBurstLauncher",    &plasmaBurstLauncher,
                      "GatlingPlasmaBurstLauncher", &gatlingPlasmaBurstLauncher,
                      "MonophasicEnergyEmitter",&monophasicEnergyEmitter,
                      "MissileLauncher",        &missileLauncher,
                      "ParticleBeamLauncher",   &particleBeamLauncher,
                      "ReinforcementBulkhead",  &reinforcementBulkhead,
                      "SelfDestructCharge",     &selfDestructCharge,
                      "CloakingDevice",         &cloakingDevice,
                      "SquareBridge",           &squareBridge,
                      "EquTBridge",             &equtBridge,
                      NULL);
  }
}
