/**
 * @file
 * @author Jason Lingle
 * @brief Contains the RunAwayCortex reflex.
 */

/*
 * c_run_away.hxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#ifndef C_RUN_AWAY_HXX_
#define C_RUN_AWAY_HXX_

#include "cortex.hxx"
#include "ci_nil.hxx"
#include "ci_self.hxx"
#include "ci_emotion.hxx"

class Ship;

/**
 * The RunAwayCortex is a reflex to try to get out of the line of fire.
 */
class RunAwayCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Emotion
<cortex_input::Nil> > {
  Ship* ship;
  float score, timeAlive;
  bool usedLastFrame;
  float lastFrameNervous;

  enum Output { throttle=0, accel, brake, spin };
  static const unsigned numOutputs = 1+(unsigned)spin;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new RunAwayCortex.
   * @param species The root of data to read from
   * @param s The ship to operate on
   * @param ss The SelfSource to use
   * @param es The EmotionSource to use
   */
  RunAwayCortex(const libconfig::Setting& species,
                Ship* s, cortex_input::SelfSource* ss,
                cortex_input::EmotionSource* es);

  /** Evaluate the outputs and control the ship, given the elapsed time. */
  void evaluate(float);

  /** This must be called every frame in which evaluate() is NOT called.
   * This allows the class to determine whether to adjust the score.
   */
  void notUsed() { usedLastFrame=false; }

  /** Returns the score of the cortex. */
  float getScore() const { return timeAlive > 0? score/timeAlive : 0; }
};


#endif /* C_RUN_AWAY_HXX_ */
