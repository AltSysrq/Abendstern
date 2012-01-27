/**
 * @file
 * @author Jason Lingle
 * @brief Contains the DodgeCortex reflex
 */

/*
 * c_dodge.hxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#ifndef C_DODGE_HXX_
#define C_DODGE_HXX_

#include "cortex.hxx"
#include "ci_self.hxx"
#include "ci_emotion.hxx"
#include "ci_nil.hxx"

class Ship;

/**
 * The DodgeCortex is a reflex which attempts to avoid taking damage.
 *
 * In any frame after using Dodge that the Ship takes damage, the
 * damage() function must be called.
 */
class DodgeCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Emotion
<cortex_input::Nil> > {
  Ship* ship;
  float score, timeAlive;

  enum Output { throttle=0, accel, brake, spin };
  static const unsigned numOutputs = 1+(unsigned)spin;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new DodgeCortex
   * @param species The root of the data to read from
   * @param s The ship to operate on
   * @param ss The SelfSource to use
   * @param es The EmotionSource to use
   */
  DodgeCortex(const libconfig::Setting& species,
              Ship* s, cortex_input::SelfSource* ss,
              cortex_input::EmotionSource* es);

  /**
   * Evaluates the Cortex to control the Ship, given the time elapsed.
   */
  void evaluate(float);

  /**
   * Notifies the Cortex of damage taken the frame after it was used.
   */
  void damage(float);

  /** Returns the score of the cortex */
  float getScore() const { return timeAlive > 0? score/timeAlive : 0; }
};

#endif /* C_DODGE_HXX_ */
