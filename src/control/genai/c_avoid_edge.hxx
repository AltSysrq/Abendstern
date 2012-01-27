/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AvoidEdgeCortex reflex
 */

/*
 * c_avoid_edge.hxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#ifndef C_AVOID_EDGE_HXX_
#define C_AVOID_EDGE_HXX_

#include "ci_nil.hxx"
#include "ci_field.hxx"
#include "ci_self.hxx"
#include "cortex.hxx"

/**
 * The AvoidEdgeCortex is a reflex which attempts to avoid flying off
 * the edge of the map.
 */
class AvoidEdgeCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Field
<cortex_input::Nil> > {
  Ship* ship;
  float score;

  enum Outputs { throttle=0, accel, brake, spin };
  static const unsigned numOutputs = 1+(unsigned)spin;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new AvoidEdgeCortex
   * @param species The root of the species to read from
   * @param s The ship to operate on
   * @param ss The SelfSource to use
   */
  AvoidEdgeCortex(const libconfig::Setting& species, Ship* s, cortex_input::SelfSource* ss);

  /**
   * Controls the ship and updates the score, given the elapsed time.
   */
  void evaluate(float);

  /**
   * Returns the score of the Cortex.
   */
  float getScore() const { return score; }
};


#endif /* C_AVOID_EDGE_HXX_ */
