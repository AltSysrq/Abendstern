/**
 * @file
 * @author Jason Lingle
 *
 * @brief Provides a number of AudioSource-compatible templates
 * that can be used to synthesise sounds.
 */

#ifndef SYNTH_HXX_
#define SYNTH_HXX_

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

#include "audio.hxx"
#include "src/globals.hxx"

namespace audio {
  /** Defines the type used to hold a one-second (for 1 Hz) table. */
  typedef Sint16 synthtab[AUDIO_FRQ];
  /** Standard synthtabs.
   * sine       One full sine wave
   * square     First half is 0x7FFF, second is -0x8000
   * ptri       Positive triangle; linear from 0x7FFF to 0x0000 then back
   * saw        First sample is -0x8000, interpolated up to 0x7FFF
   * decay      First sample is 0x7FFF, interpolated down to 0x0000
   * build      Reverse of decay
   */
  extern synthtab sinetab, squaretab, sawtab, ptritab, decaytab, buildtab;

  /** Ensures that the synthtabs have been generated.
   * This is called automatically by the Playtab class and internally.
   */
  void ensureSythtabsGenerated();

  /** Plays a synthtab.
   * @param Tab defines which table to play through.
   * @param Speed defines how quickly we pass through the table, in Hz
   * @param Repeat if true, we go back to the beginning after a playthrough;
   *   otherwise, we zero the rest of the buffer out and return false.
   * @param Begin defines the first sample offset
   * @param End defines what we consider to be the end of the table.
   *
   * This is a mono-only class.
   */
  template<const synthtab& Tab, unsigned Speed=1, bool Repeat=true,
           unsigned Begin = 0, unsigned End = AUDIO_FRQ>
  class Playtab {
    unsigned off;

    public:
    Playtab()
    : off(Begin)
    { ensureSythtabsGenerated(); }

    inline bool get(Sint16* dst, unsigned cnt) {
      Sint16* dstend = dst+cnt;
      copy:
      for (; off < End && dst != dstend; off += Speed)
        *dst++ = Tab[off];

      if (dst != dstend) {
        //We hit the end
        if (Repeat) {
          off -= (End-Begin);
          //off = Begin;
          goto copy;
        } else {
          std::memset(dst, 0, 2*(dstend-dst));
          return false;
        }
      }

      return true;
    }
  };

  /** Internal use only. #undefed at end of file.
   * Ensures that rand() returns 16-bits of randomness.
   */
  #if RAND_MAX >= 0xFFFF
  #define RAND16 ((Sint16)(std::rand() & 0xFFFF))
  #else
  #define RAND16 ((Sint16)(((std::rand() << 8) ^ (std::rand())) & 0xFFFF))
  #endif

  /** Generates off-white noise.
   * The frequency range is between 0 and AUDIO_FRQ/FrqDown; increasing
   * FrqDown will reduce the perceived pitch of the noise. FrqDown works
   * by giving that many consecutive samples the same value.
   * By default, it will have 100% volume, unless VolDiv is non-one.
   * This is a mono-only class by design; however, it will still produce
   * expected results for stereo output (in this case, FrqDown will
   * determine how many consecutive half-samples have the same value).
   */
  template<unsigned FrqDown=1, unsigned VolDiv=1>
  class WhiteNoise {
    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      unsigned i;
      //We need to stop early so we can gracefully handle
      //the case where the buffer size is not an even
      //multiple of FrqDown
      for (i=0; i + FrqDown <= cnt; i += FrqDown) {
        Sint16 val = RAND16/VolDiv;
        for (unsigned j=0; j<FrqDown; ++j)
          dst[i+j] = val;
      }
      //Handle the last few values specially
      if (i < cnt) {
        Sint16 val = RAND16/VolDiv;
        while (i<cnt) dst[i++] = val;
      }
      return true;
    }
  };

  /** Generates brown noise.
   * The pitch can be lowered by increasing FrqDown, which works the same
   * way as for WhiteNoise.
   * VolDiv also works as in WhiteNoise, but, since it is used for the /difference/
   * between the previous and current values, it generally should not be less than 4.
   * This is a mono-only class by design; however, it will produce
   * expected results for stereo output.
   */
  template<unsigned FrqDown=1, unsigned VolDiv=4>
  class BrownNoise {
    signed prv; //This MUST be bigger than 16-bit, since we handle clipping properly

    public:
    BrownNoise() : prv(0) {}
    inline bool get(Sint16* dst, unsigned cnt) {
      unsigned i;
      for (i=0; i + FrqDown <= cnt; i += FrqDown) {
        Sint16 delta = RAND16/VolDiv;
        //Prevent clipping
        if (prv + delta < -0x8000 || prv + delta > 0x7FFF)
          delta = -delta;
        Sint16 val = (Sint16)(prv + delta);
        prv = val;
        for (unsigned j=0; j<FrqDown; ++j)
          dst[i+j] = val;
      }

      //Just fill the rest in with the previous
      while (i < cnt) dst[i++] = prv;

      return true;
    }
  };

  /** Simply plays silence.
   * Channel-agnostic.
   */
  class Silence {
    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      std::memset(dst, 0, 2*cnt);
      return true;
    }
  };

  /* *Multiplies two inputs together.
   * This class is channel-agnostic.
   * Returns false if either of the children do.
   */
  template<typename A, typename B>
  class Mult {
    A a;
    B b;

    Sint16 tmp[AUDIO_FRQ*2];

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      //Note the use of & instead of && (we MUST get both, regardless
      //of a's return)
      bool ret = a.get(dst,cnt) & b.get(tmp,cnt);
      for (unsigned i=0; i<cnt; ++i)
        muleq(dst[i], tmp[i]);
      return ret;
    }
  };

  /** Adds two inputs together.
   * Does not handle clipping.
   * This class in channel-agnostic.
   * Returns false if either of the children do.
   */
  template<typename A, typename B>
  class Add {
    A a;
    B b;

    Sint16 tmp[AUDIO_FRQ*2];

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      //Note the use of & instead of && (we MUST get both, regardless
      //of a's return)
      bool ret = a.get(dst,cnt) & b.get(tmp,cnt);
      for (unsigned i=0; i<cnt; ++i)
        dst[i] += tmp[i];
      return ret;
    }
  };

  /** Mixes two inputs together, by first dividing both inputs by two.
   * Channel-agnostic.
   */
  template<typename A, typename B>
  class Mix2 {
    A a;
    B b;
    Sint16 tmp[AUDIO_FRQ*2];

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      bool ret = a.get(dst,cnt) & /* not && */ b.get(tmp,cnt);
      for (unsigned i=0; i<cnt; ++i) {
        signed m = dst[i];
        m += tmp[i];
        if (m < -0x8000)     dst[i] = -0x8000;
        else if (m > 0x7FFF) dst[i] =  0x7FFF;
        else                 dst[i] =  m;
      }
      return ret;
    }
  };

  /** Frequency-modulates the first input by the second.
   * The absolute-value of the second input is used to determine
   * the speed of input for the second, where 0 = 0 and 0x8000 = 1.0.
   * This is mono-only.
   */
  template<typename A, typename B>
  class Modulate {
    A a;
    B b;

    Sint16 abuff[1024];
    /* We use a fixed-point value here, where the upper 17 bits are the
     * index into abuff and the lower 15 are the suboffset.
     */
    unsigned aoff; //Offset << 15
    bool hasMoreA;

    public:
    Modulate() : aoff(lenof(abuff) << 15), hasMoreA(true) { }

    inline bool get(Sint16* dst, unsigned cnt) {
      //We can override B's input as we determine A's input
      bool hasMoreB = b.get(dst,cnt);
      for (unsigned i=0; i<cnt; ++i) {
        if ((aoff >> 15) >= lenof(abuff)) {
          if (hasMoreA) {
            //We need more from A
            hasMoreA = a.get(abuff, lenof(abuff));
            aoff &= 0x7FFF; //Remove everything other than the fractional
          } else {
            //We are completely out of input
            std::memset(dst+i, 0, 2*(cnt-i));
            return false;
          }
        }

        unsigned delta = dst[i];
        delta &= 0xFFFF;
        if (delta & 0x8000) delta ^= 0xFFFF;
        dst[i] = abuff[aoff >> 15];
        aoff += delta;
      }

      return hasMoreB;
    }
  };

  /** Reduces the volume of the input by the given amount.
   * Channel-agnostic.
   */
  template<typename A, Sint16 VolMul>
  class VolumeDown {
    A a;

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      bool ret = a.get(dst, cnt);
      for (unsigned i=0; i<cnt; ++i)
        muleq(dst[i], VolMul);
      return ret;
    }
  };

  /** Like VolumeDown, but takes a dynamic volume and is an AudioSource.
   */
  template<typename A>
  class DVolumeDown: public AudioSource {
    A a;
    Sint16 mul;

    public:
    DVolumeDown(Sint16 m)
    : mul(m) {}
    virtual bool get(Sint16* dst, unsigned cnt) {
      bool ret = a.get(dst,cnt);
      for (unsigned i=0; i<cnt; ++i)
        muleq(dst[i], mul);
      return ret;
    }
  };

  /** Transforms a -0x8000..+0x7FFF stream to 0x0000..0x7FFF.
   * Channel-agnostic.
   */
  template<typename A>
  class MakePositive {
    A a;

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      bool ret = a.get(dst, cnt);
      Uint16* udst = (Uint16*)dst;
      for (unsigned i=0; i<cnt; ++i) {
        udst[i] += 0x8000;
        udst[i] >>= 1;
      }

      return ret;
    }
  };

  /** Adjusts the speed of the input stream by an arbitrary
   * amount. The speed multiplier is a fixed-point represented
   * as an integer 32768 times the desired speed.
   * This class is mono-only.
   */
  template<typename A, unsigned Factor>
  class AdjustSpeed {
    A a;

    Sint16 tmp[1024];
    unsigned off;
    bool hasMoreA;

    public:
    AdjustSpeed() : off(lenof(tmp) << 15), hasMoreA(true) {}
    inline bool get(Sint16* dst, unsigned cnt) {
      for (unsigned i=0; i<cnt; ++i) {
        if ((off >> 15) >= lenof(tmp)) {
          if (hasMoreA) {
            hasMoreA = a.get(tmp, lenof(tmp));
            off -= lenof(tmp) << 15;
          } else {
            //No more input, fill with zeros and return
            std::memset(dst+i, 0, (cnt-i)*2);
            return false;
          }
        }
        dst[i] = tmp[off >> 15];
        off += Factor;
      }
      return true;
    }
  };

  /** Duplexes a mono stream to stereo. */
  template<typename A>
  class Duplex {
    A a;

    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      //If we put data in at the middle first, we can do the entire
      //process in-place
      bool ret = a.get(dst+cnt/2,cnt/2);
      for (unsigned i=0, j=cnt/2; i<cnt; ++j) {
        dst[i++] = dst[j];
        dst[i++] = dst[j];
      }

      return ret;
    }
  };

  /** Limits the input to the specified duration. If the input
   * is shorter, this returns false earlier.
   */
  template<typename A, unsigned Samples>
  class Duration {
    A a;
    signed samples;

    public:
    Duration() : samples(Samples) {}
    inline bool get(Sint16* dst, unsigned cnt) {
      samples -= cnt;
      return a.get(dst,cnt) && samples > 0;
    }
  };

  /* Fills the buffer with the specified value. */
  template<Sint16 v>
  class Value {
    public:
    inline bool get(Sint16* dst, unsigned cnt) {
      while (cnt--)
        *dst++ = v;
      return true;
    }
  };

  /** Simplifies toggling an effect within a mixer.
   * Wraps the input in an internal class type that gets automatically
   * added to the mixer when created. The on/off state can be changed
   * at any time; any creation/termination is handled automatically.
   * Channel-agnostic.
   * If the child terminates, the stream is still considered "on" and
   * must be toggled to restart.
   */
  template<typename A>
  class Toggle {
    class Helper: public AudioSource {
      A a;
      Toggle& parent;

      public:
      bool on;

      Helper(Toggle& par)
      : parent(par), on(true) {}

      virtual bool get(Sint16* dst, unsigned cnt) {
        if (a.get(dst,cnt))
          return on;
        else {
          parent.terminated();
          return false;
        }
      }
    }* helper;
    bool isOn;

    Mixer& mixer;

    public:
    Toggle(Mixer& mx)
    : helper(NULL), isOn(false), mixer(mx) {}

    ~Toggle() { setOn(false); }

    bool getOn() const { return isOn; }
    void setOn(bool on) {
      if (on == isOn) return;
      if (on) {
        helper = new Helper(*this);
        mixer.add(helper);
      } else {
        if (helper) helper->on = false;
        helper = NULL;
      }
      isOn = on;
    }

    /* Only to be used by Helper */
    void terminated() {
      helper = NULL;
    }
  };

  /** Plays one input, then the other after that's finished.
   * Channel-agnostic.
   */
  template<typename A, typename B>
  class Seq {
    A a;
    B b;
    bool hasMoreA;

    public:
    Seq() : hasMoreA(true) {}
    inline bool get(Sint16* dst, unsigned cnt) {
      if (hasMoreA) {
        hasMoreA = a.get(dst,cnt);
        return true;
      } else {
        return b.get(dst, cnt);
      }
    }
  };

  /** Plays one input for exactly the number of samples specified,
   * then the next (forever).
   * Channel-agnostic.
   */
  template<typename A, unsigned Alen, typename B>
  class NSeq {
    A a;
    B b;
    unsigned aix;

    public:
    NSeq() : aix(0) {}
    inline bool get(Sint16* dst, unsigned cnt) {
      if (aix == Alen)
        return b.get(dst,cnt);
      else if (aix+cnt <= Alen) {
        a.get(dst,cnt);
        aix += cnt;
        return true;
      } else {
        //We need to use both
        a.get(dst,Alen-aix);
        dst += Alen-aix;
        cnt -= Alen-aix;
        aix = Alen;
        return b.get(dst,cnt);
      }
    }
  };
  /** Plays the input over and over forever (by reinstantiating
   * the templated class).
   */
  template<typename A>
  class Loop {
    char adat[sizeof(A)];
    A*const a;

    public:
    Loop() : a((A*)&adat) { new (a) A; }
    inline bool get(Sint16* dst, unsigned cnt) {
      if (!a->get(dst,cnt)) {
        a->A::~A();
        new (a) A;
      }
      return true;
    }

    ~Loop() {
      a->A::~A();
    }
  };

  #undef RAND16
};

#endif /* SYNTH_HXX_ */
