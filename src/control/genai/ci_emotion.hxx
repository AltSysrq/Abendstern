/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Emetion cortex input provider.
 */

/*
 * ci_emotion.hxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#ifndef CI_EMOTION_HXX_
#define CI_EMOTION_HXX_

#include <map>
#include <string>

#include "src/opto_flags.hxx"

class GameField;
class Ship;

namespace cortex_input {
  /**
   * Used by the Emotion input provider.
   * Performs actual calculations of the various emotions.
   * It must be noted that this class has an update(float) function,
   * which must be manually called by the GenAI class (which should be the
   * only one to extend this one).
   *
   * This class also the virtual damage() function from Controller.
   */
  class EmotionSource {
    float timeUntilRescan;
    GameField* field;
    //Despite being private, this still somehow pollutes the namespace
    //of GenAI ("ship is ambiguous: candidates are Controller::ship,
    //SelfSourte::ship, EmotionSource::ship")
    Ship* esship;
    //For the persistent pain, keep track of total pain in eight octants,
    //and use the maximum for the actual input.
    //Divided by (minimum) and reported direction:
    //  0 -π1     -7π/8
    //  1 -3pi/4  -5π/8
    //  2 -π/2    -3π/8
    //  3 -π/4    -1π/8
    //  4 0       +1π/8
    //  5 +π/4    +3π/8
    //  6 +π/2    +5π/8
    //  7 +3π/4   +7π/8
    float cumulativeDamage[8];

    public:
    /** See Controller::damage() */
    virtual void damage(float,float,float) noth;
    virtual ~EmotionSource() {}

    /** Offsets of emotional inputs provided. */
    struct InputOffset {
      enum t {
        nervous=0, nervoust, fear, feart,
        painta, paintt, painpa, painpt
      };
    };
    /** The number of inputs provided. */
    static const unsigned escip_numInputs = 1+(unsigned)InputOffset::painpt;

    private:
    float inputs[escip_numInputs];

    protected:
    /**
     * Constructs the EmotionSource operating on the given Ship.
     */
    EmotionSource(Ship*);

    /** Updates the inputs based on the new state.
     * Subclasses must make sure to call this.
     */
    void update(float) noth;

    public:
    /** Copies inputs into the destination array. */
    void getInputs(float* dst) const noth;
  };

  /**
   * Uses an EmotionSource to provide the various emotional inputs to the
   * Cortex.
   */
  template<typename Parent>
  class Emotion: public Parent {
    const EmotionSource* source;

    protected:
    static const unsigned cip_first = Parent::cip_last;
    static const unsigned cip_last = cip_first + EmotionSource::escip_numInputs;
    static const unsigned
      cip_nervous       = cip_first + (unsigned)EmotionSource::InputOffset::nervous,
      cip_nervoust      = cip_first + (unsigned)EmotionSource::InputOffset::nervoust,
      cip_fear          = cip_first + (unsigned)EmotionSource::InputOffset::fear,
      cip_feart         = cip_first + (unsigned)EmotionSource::InputOffset::feart,
      cip_painta        = cip_first + (unsigned)EmotionSource::InputOffset::painta,
      cip_paintt        = cip_first + (unsigned)EmotionSource::InputOffset::paintt,
      cip_painpa        = cip_first + (unsigned)EmotionSource::InputOffset::painpa,
      cip_painpt        = cip_first + (unsigned)EmotionSource::InputOffset::painpt;

    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(nervous);
        I(nervoust);
        I(fear);
        I(feart);
        I(painta);
        I(painpa);
        I(paintt);
        I(painpt);
        #undef I
      }
    };

    /** Sets the source to use for inputs.
     * This MUST be called before bindInputs(float).
     */
    void setEmotionSource(const EmotionSource* emo) {
      source = emo;
    }

    void bindInputs(float* dst) noth {
      Parent::bindInputs(dst);
      source->getInputs(dst+cip_first);
    }
  };
}

#endif /* CI_EMOTION_HXX_ */
