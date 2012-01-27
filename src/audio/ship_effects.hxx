/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines all sound effects associated with ships, as well as a
 * function to manage the ambients.
 */

#ifndef SHIP_EFFECTS_HXX_
#define SHIP_EFFECTS_HXX_

#include "ship_mixer.hxx"
#include "ship_effects_common.hxx"
#include "synth.hxx"

class Ship;

namespace audio {
  /** General background noise for the ship -- a quiet brown noise comming from every cell.
   */
  extern ShipAmbient shipBackground;

  /** Noise produced by capacitors.
   * There are 31 ambient effects, starting
   * from a quiet, low tone, rising to a medium-volume, high tone.
   * Each index corresponds to the capacitance percent (index/32.0f).
   * Index 0 is NULL, as that level is just silence.
   */
  extern ShipAmbient*const capacitorRing[32];

  /** Noise produced by FissionPower. */
  extern ShipAmbient fissionPower;

  /** Noise produced by FusionPower */
  extern ShipAmbient fusionPower;

  /** Noise produced by AntimatterPower */
  extern ShipAmbient antimatterPower;

  /** Engine noise for ParticleAccelerator (not Super). */
  extern ShipAmbient particleAccelerator;

  /** Engine noise for MiniGravwaveDrive (not MKII) */
  extern ShipAmbient miniGravwaveDrive;

  /** Engine noise for BussardRamjet */
  extern ShipAmbient bussardRamjet;

  /** Engine noise for SuperParticleAccelerator */
  extern ShipAmbient superParticleAccelerator;

  /** Engine noise for MiniGravwaveDriveMKII */
  extern ShipAmbient miniGravwaveDriveII;

  /** Engine noise for RelIonAccelerator */
  extern ShipAmbient relIonAccelerator;

  /** Noise produced by ShieldGenerator */
  extern ShipAmbient shieldGenerator;

  /** Firing-rev sound for non-turbo GatlingPlasmaBurstLauncher */
  extern ShipAmbient gatPlasmaBurstLauncherRevNorm;
  /** Firing-rev sound for turbo GatlingPlasmaBurstLauncher */
  extern ShipAmbient gatPlasmaBurstLauncherRevTurbo;

  /** Firing sounds for EnergyChargeLauncher */
  extern ShipStaticEvent energyChargeLauncher0, energyChargeLauncher1; ///< Secondary ECL sound
  /** Only start the energyChargeLauncher effect if this is true */
  extern bool energyChargeLauncherOK;

  /** Firing sounds for MagnetoBombLauncher */
  extern ShipStaticEvent magnetoBombLauncher0,
                         magnetoBombLauncher1, ///< Volume based on level (5..9 = 100%, 1=20%, etc)
                         magnetoBombLauncherOP; ///< Sound to play on overpowered launch (6=40%, 9=100%)
  /** Only start the magnetoBombLauncher effect if this is true */
  extern bool magnetoBombLauncherOK;

  /** Firing sounds for SemiguidedBombLauncher */
  extern ShipStaticEvent sgBombLauncher0,
                         sgBombLauncher1, ///< Volume based on level (5..9 = 100%, 1=20%, etc)
                         sgBombLauncher2, ///< Fixed volume
                         sgBombLauncherOP; ///< Sound to play on overpowered launch (6=40%, 9=100%)
  /** Only start the sgBombLauncher effect if this is true */
  extern bool sgBombLauncherOK;

  /** Firing sound for PlasmaBurstLauncher */
  extern ShipStaticEvent plasmaBurstLauncher;
  /** Only start the plasmaBurstLauncher effect if this is true */
  extern bool plasmaBurstLauncherOK;

  /** Firing sound for non-turbo GatlingPlasmaBurstLauncher */
  extern ShipStaticEvent gatPlasmaBurstLauncherNorm;
  /** Only start gatPlasmaBurstLauncherNorm if this is true */
  extern bool gatPlasmaBurstLauncherNormOK;
  /** Firing sound for turbo GatlingPlasmaBurstLauncher */
  extern ShipStaticEvent gatPlasmaBurstLauncherTurbo;
  /** Only start gatPlasmaBurstLauncherTurbo if this is true */
  extern bool gatPlasmaBurstLauncherTurboOK;

  /** Firing sound for MissileLauncher */
  extern ShipStaticEvent missileLauncher;
  /** Only start the missileLauncher effect if this is true */
  extern bool missileLauncherOK;

  /** Firing sound for MonophasicEnergyEmitter */
  extern ShipStaticEvent monophasicEnergyEmitter;
  /** Only start the monophasicEnergyEmitter effect if this is true */
  extern bool monophasicEnergyEmitterOK;

  /** Firing sound for ParticleBeamLauncher (all forms) */
  extern ShipStaticEvent particleBeamLauncher;
  /** Only start the particleBeamLauncher effect if this is true */
  extern bool particleBeamLauncherOK;

  /** Sound for damaging impacts */
  typedef Modulate<WhiteNoise<8,1>, Mult<Playtab<sinetab,16>,Playtab<decaytab,1> > > shipImpact;
  /** The length of the shipImpact sound */
  #define SHIP_IMPACT_LEN 22050

  /** Returns an AudioSource for the shield impact sound */
  AudioSource* shieldImpactSound();
  /** Returns the length of the shield impact sound */
  unsigned shieldImpactLength();
}

#endif /* SHIP_EFFECTS_HXX_ */
