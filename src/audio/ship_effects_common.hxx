/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines the one ShipMixer-related function the Ship header must know about.
 *
 * This is done so that not everything which includes ship.hxx will
 * include the entire audio system.
 *
 * The contents are implemented with the rest of ShipMixer in src/audio/ship_mixer.cxx.
 * @see src/audio/ship_mixer.hxx
 */

#ifndef SHIP_EFFECTS_COMMON_HXX_
#define SHIP_EFFECTS_COMMON_HXX_

class Ship;
namespace audio {
  /** Updates current effects for the given ship.
   * This MUST be a friend of class Ship.
   * It should not be called by anyone outside of Ship.
   */
  void shipSoundEffects(float,Ship*);
}

#endif /* SHIP_EFFECTS_COMMON_HXX_ */
