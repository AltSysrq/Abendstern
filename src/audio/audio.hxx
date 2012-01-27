/**
 * @file
 * @author Jason Lingle
 *
 * @brief Provides the base interface to Abendstern's audio system.
 *
 * This file provides the basic framework for management,
 * playback, synthesis, and manipulation of audio.
 *
 * Internally, everything uses signed native-endian 16-bit
 * audio at AUDIO_FRQ Hz. Depending on context, both sterio
 * and mono are used.
 *
 * The audio system is arranged into a tree of classes (mostly
 * templates) which have a<br>
 * &nbsp;  <code>bool get(Sint16* dat, Sint16* tmp, unsigned cnt)</code><br>
 * function that generates the next cnt Uint16s of data (regardless
 * of sample format) and returns whether the source still exists.
 */

#ifndef AUDIO_HXX_
#define AUDIO_HXX_

#include <vector>

//Include open GL to make MSVC happy (since it can't be
//included AFTER SDL.h is included).
#include <GL/gl.h>
#include <SDL.h>

/**
 * The sample frequency used.
 */
#define AUDIO_FRQ 22050

/**
 * @brief Contains all declarations for working with audio.
 *
 * @see Description of src/audio/audio.hxx
 */
namespace audio {
  /** The AudioSource is an abstract class that provides an abstract get function.
   * Its sole purpose is to generalise the templates' interfaces.
   */
  class AudioSource {
    public:
    /** Retrieves a block of audio information.
     * The exact number of bytes in the destination buffer must be set
     * by the implementation. If the implementation cannot provide that
     * many samples, the rest must be zeroed out.
     * @param dst The destination buffer
     * @param cnt The length of dst, in Sint16s (NOT samples)
     * @return Whether the AudioSource will be able to return further data in another call.
     */
    virtual bool get(Sint16* dst, unsigned cnt) = 0;
    virtual ~AudioSource() {}
  };

  /** The Virtual template takes an appropriate type and makes it into an AudioSource.
   * @param T Any type that provides a member bool get(Sint16*,unsigned).
   */
  template<typename T>
  class Virtual: public AudioSource, public T {
    public:
    virtual bool get(Sint16* d, unsigned c) {
      return T::get(d,c);
    }
  };

  /** Mixes multiple AudioSources into one stream.
   * The Mixer class takes any number of AudioSources and combines
   * them into one, optionally reducing the overall volume at the
   * same time.
   *
   * The "nominal count" is the number of inputs to simply add together
   * before decreasing the volume to reduce clipping effects.
   * The default is high enough that no reduction will be performed.
   *
   * Since this class is dynamic, it takes AudioSources instead
   * of templated types.
   *
   * It is not, however, itself an AudioSource, in order to avoid
   * the virtual function call when possible.
   *
   * The Mixer is channel-count-agnostic; it only requires that
   * the number of channels in all inputs is the same.
   *
   * When an AudioSource::get returns false, it is removed and
   * deleted. Its ::get always returns true.
   *
   * The root Mixer's output is directly given to SDL.
   */
  class Mixer {
    protected:
    std::vector<AudioSource*> sources;
    Sint16 tmp[AUDIO_FRQ*2];

    public:
    /** The "nominal count" of inputs.
     * May be set to any non-zero value whenever it can be known that
     * get() will not be called concurrently.
     */
    unsigned nominalCount;
    /** The volume reduction multiplier.
     * May be set to any value whenever it can be known that
     * get() will not be called concurrently.
     */
    Sint16 volume;

    /** Constructs a new mixer with the given initial values.
     * @param nominalCount The number of sources to allow before beginning to reduce overall volume.
     * @param baseVolumeMul The initial value of volume.
     */
    Mixer(unsigned nominalCount = 0x4000, Sint16 baseVolumeMul = 0x7FFF);
    bool get(Sint16*, unsigned);

    /** Adds the specified source to the mixer.
     * After this call, the Mixer permanently owns this source, and
     * will delete it when its life expires or the Mixer's destructor
     * runs.
     * This can only be called when it can be guaranteed that get() will
     * not be called concurrently.
     */
    void add(AudioSource*);

    /** Returns the number of sources contained. */
    unsigned size() const { return sources.size(); }

    ~Mixer();
  } extern trueRoot; ///< Root mixer that is direct input for SDL.

  /** Simple subclass of Mixer that makes it an AudioSource. */
  class VMixer: public Mixer, public AudioSource {
    public:
    /** Same as Mixer::Mixer */
    VMixer(unsigned nominalCount = 0x4000, Sint16 baseVolumeMul = 0x7FFF)
    : Mixer(nominalCount,baseVolumeMul)
    { }

    virtual bool get(Sint16* dst, unsigned cnt) {
      return Mixer::get(dst,cnt);
    }
  } extern &root; ///< Main root for most sounds (reduced volume)

  /** Initialises the audio system and immediately starts playing (silence). */
  void init();

  /** Terminates the audio system. */
  void term();

  /** Scale-multiplies two Sint16s and returns the result.
   * This is essentially treating one as a fractional value between
   * -1 and +1.
   */
  static inline Sint16 mul(Sint16 a, Sint16 b) {
    /* Shifting both right then multiplying discards too much data
     * (though it is faster), which is a severe problem for ship
     * effects, which often only use the lower 7 bits (which, again,
     * end up discarded).
     */
    Sint32 r = a;
    r *= b;
    return (r >> 15);
  }

  /** Same as mul(), but overwrite a with the result. */
  static inline void muleq(Sint16& a, Sint16 b) {
    a = mul(a,b);
  }

  /** Returns a positive fractional value from the given numerator and denominator.
   * (ie, the result is between 0 and 0x7FFF for 0..1).
   */
  static inline Sint16 frac(unsigned n, unsigned d) {
    n <<= 15;
    n /= d;
    if (n & 0x8000) n = 0x7FFF; //Ensure we don't get 0x8000, which is negative
    return (Sint16)n;
  }
}

#endif /* AUDIO_HXX_ */
