/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ReflexCortex
 */

/*
 * c_reflex.hxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#ifndef C_REFLEX_HXX_
#define C_REFLEX_HXX_

#include "cortex.hxx"
#include "ci_self.hxx"
#include "ci_field.hxx"
#include "ci_emotion.hxx"
#include "ci_nil.hxx"

class Ship;

/**
 * The ReflexCortex evaluates emotions and decides whether to
 * proceed normally, avoid the edge, dodge, or run away.
 */
class ReflexCortex:
public Cortex,
private
 cortex_input::Self
<cortex_input::Emotion
<cortex_input::Field
<cortex_input::Nil> > > {
  enum Outputs { edge=0, dodge, runaway };
  static const unsigned numOutputs = 1+(unsigned)runaway;

  float score, timeAlive;

  public:
  typedef cip_input_map input_map;

  /** Possible directives emitted by this Cortex. */
  enum Directive {
    Frontal, ///Proceed with standard processing with the FrontalCortex
    AvoidEdge, ///Use the AvoidEdgeCortex reflex
    Dodge, ///Use the DodgeCortex reflex
    RunAway ///Use the RunAwayCortex reflex
  };

  /**
   * Constructs a new ReflexCortex operating for the given Ship.
   * @param species Setting root of the species used
   * @param s The ship to operate on
   * @param ss The SelfSource object for the Self variables
   * @param es The EmotionSource object for the Emotion variables
   */
  ReflexCortex(const libconfig::Setting& species,
               Ship* s, cortex_input::SelfSource* ss, cortex_input::EmotionSource* es);

  /** Evaluates the various outputs and determines what to do.
   * Scoring is updated according to the elapsed time specified.
   */
  Directive evaluate(float);

  /** Returns the score of the score of the cortex. */
  float getScore() const { return score; }
};


#endif /* C_REFLEX_HXX_ */
