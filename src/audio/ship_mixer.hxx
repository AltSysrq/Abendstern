/**
 * @file
 * @author Jason Lingle
 *
 * @brief Provides the interface to the Ship sound effect mixing system.
 *
 * The ship mixer system uses the physical structure of the player's
 * ship to produce realistic sounds for the systems and external events.
 *
 * The Ship is modelled as a graph of nodes; all connections are treated
 * as equidistant, even though this is not exactly the case. These nodes
 * are derived from the Cells, though EmptyCells are not included.
 *
 * An inverse-speed-of-sound -- in samples per cell -- is calculated based
 * on the ship's reinforcement; higher reinforcement leads to higher speed
 * of sound (and lower inverse speed) and lower power loss per node traversal.
 *
 * Each cell-node in the graph has a table-of-conduction calculated, which
 * describes a list of times that any sound will reach the bridge: the delay
 * in samples, the volume reduction, and which channels it affects. This is
 * determined by running a simple simulation over the ship for that cell.
 *
 * Any incomming sounds are remixed using the tables-of-conduction from the
 * source cells. This will lead to amplification of the ship's resonant
 * frequencies, as well as accurate delay and panning information.
 *
 * Sounds are divided into three categories:
 * + Ambient/togglable. These are sounds that are usually produced continuously.
 *   A very short input sample (which can be seamlessly looped) is used as input,
 *   as well as a number of cell sources. Three pieces of audio are generated from
 *   this: the initial, which is used as a transition from silence to full resonance,
 *   the middle, which is looped from after the initial is done to when the sound is
 *   terminated, and the final, which plays immediately after termination.
 *   Some ambients which have multiple varieties of which only one at a time ever plays,
 *   such as that for capacitors, will not have the initial/finals.
 * + Static events. These are sounds that are produced once at a time, but many times,
 *   such as weapon-firing sounds. They take an input sample of the entire effect, as
 *   well as a number of cell sources. The conduction tables are used to remix this into
 *   a single sound sample, which is played on-demand.
 * + Dynamic events. These are sounds that occur rarely and in specific ways, such as
 *   ship damage effects. They take an AudioSource as input, as well as a single cell
 *   source, and are remixed in real time according to conduction tables.
 *
 * Many of the calculations performed are quite expensive. As such, they are performed
 * on a dedicated thread.
 *
 * The interface in this file can only be used for a single ship at a time. This limitation
 * is not meaningful, though, since this is realistic in space. Additionally, it would be
 * difficult for a computer to perform these calculations for more ships.
 */

#ifndef SHIP_MIXER_HXX_
#define SHIP_MIXER_HXX_

#include <GL/gl.h>
#include <SDL.h>

#include "audio.hxx"

class Cell;

namespace audio {
  /** A workaround for MSVC++.
   *
   * When MSVC++ sess extern Foo*const without initialisation, it complains
   * about a const not being initialised or without extern (though extern
   * is obviously there). However, without const, the code compiles and
   * links the way we expect.
   */
  #ifndef WIN32
  #define SHIP_MIXER_CONST const
  #else
  #define SHIP_MIXER_CONST
  #endif

  /** Starts a new dynamic effect.
   * @param src AudioSource to use for input
   * @param len Number of values to read in
   * @param cell Single cell source location
   * @param Volume multiplier; a fudge-factor to increase/decrease the
   *   volume independent of the input data or ship acoustics
   */
  //This needs to come before the ShipMixer class, since apparently MSVC++
  //can't handle the friend declaration otherwise
  void shipDynamicEvent(AudioSource* src, unsigned len, Cell* cell, float volmul);

  /** A special Mixer that adds some required multithreading synchronisation.
   * Also provides the main interface to the sound effect backend.
   */
  class ShipMixer: public AudioSource, private Mixer {
    friend struct ShipAmbient_impl;
    friend struct ShipStaticEvent_impl;
    friend void audio::shipDynamicEvent(AudioSource*,unsigned,Cell*,float);
    public:
    virtual ~ShipMixer() {}

    virtual bool get(Sint16* s,unsigned u);

    /** Begins the worker thread for the ShipMixer.
     * This MUST be called before anything else related to this
     * file will work.
     */
    static void init();

    /** Stops all playing sounds and discards all information about
     * the current ship.
     */
    static void reset();

    /** Resets and terminates the worker thread.
     * This should be called before quitting the application, and when
     * exiting contexts where the ShipMixer is necessary.
     */
    static void end();

    /** Remaps the current ship.
     * Until the new calculations are complete,
     * the previous ship (if there was one) will be used.
     * When switching to entirely different ships, the reset() function
     * should be called first.
     *
     * Any Cell*s not found in the provided array but do occur in ambient
     * and static effects are removed from those.
     */
    static void setShip(Cell*const*, unsigned cnt, float reinforcement);
  } /** The singleton instance of ShipMixer. */ extern *SHIP_MIXER_CONST shipMixer;

  struct ShipAmbient_impl;
  /** Defines an ambient sound type.
   * The constructor of this class MUST be called when the ShipMixer thread
   * is NOT running (ie, it should be a static or auto global object).
   * Additionally, this class does NOT free its allocated resources; in fact,
   * freeing it simply relinquishes control over the effect, which will persist.
   */
  class ShipAmbient {
    friend class ShipMixer;

    ShipAmbient_impl* dat;

    public:
    /** Constructs the ambient effect with the given audio source and number of samples to read from it.
     * The fourth and fifth arguments indicate how to treate the init
     * and final sections. Values of 0 indicates to not generate them.
     * 1 indicates to use the same data from the middle (without rereading
     * from the source); anything else indicates to read that many samples
     * from the source. In this case, the order is initial, main, final.
     * @param src AudioSource to use for input
     * @param volmul A value to multiply the input volumes by
     * @param midlen Indicates the length of the middle section.
     * @param inlen See fourth and fifth arguments above.
     * @param finlen See fourth and fifth arguments above.
     */
    ShipAmbient(AudioSource* src, float volmul, unsigned midlen, unsigned inlen = 1, unsigned finlen = 1);

    /** Sets the state of the effect. 0 = off, anything else is volume.
     * This function must ONLY be called between calls to
     * SDL_LockAudio and SDL_UnlockAudio.
     */
    void set(Sint16);

    /** Adds the given Cell* to the list of sources.
     * This call may only be performed while no ship is registered
     * (ie, before the first ever call to setShip or between reset and
     * setShip, in class ShipMixer).
     */
    void addSource(Cell*);
  };

  struct ShipStaticEvent_impl;
  /** Defines a static event type.
   * The constructor of this class MUST be called when the ShipMixer thread
   * is NOT running (ie, it should be a static or auto global object).
   * Additionally, this class does NOT free its allocated resources; in fact,
   * freeing it simply relinquishes control over the effect, which will persist.
   */
  class ShipStaticEvent {
    friend class ShipMixer;

    ShipStaticEvent_impl* dat;

    public:
    /** Constructs the static effect with the given audio source.
     * @param src AudioSource to use for input
     * @param volmul Amount to multiply volumes by
     * @param duration Number of values to read from src
     */
    ShipStaticEvent(AudioSource* src, float volmul, unsigned duration);

    /** Begins playing the effect at the specified relative volume.
     * A new sound is added if it is already playing.
     */
    void play(Sint16 = 0x7FFF);

    /** Adds the given Cell* to the list of sources.
     * This call may only be performed while no ship is registered
     * (ie, before the first ever call to setShip or between reset and
     * setShip, in class ShipMixer).
     */
    void addSource(Cell*);
  };
}

#endif /* SHIP_MIXER_HXX_ */
