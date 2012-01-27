/**
 * @file
 * @author Jason Lingle
 * @brief Contains the NavigationCortex
 */

/*
 * c_navigation.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_NAVIGATION_HXX_
#define C_NAVIGATION_HXX_

#include "cortex.hxx"
#include "ci_nil.hxx"
#include "ci_self.hxx"
#include "ci_objective.hxx"

class Ship;
class GameObject;

/**
 * The NavigationCortex attempts to navigate to the objective, matching
 * location and velocity.
 */
class NavigationCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Objective
<cortex_input::Nil> > {
  Ship*const ship;
  float score;

  enum Output { throttle=0, accel, brake, spin };
  static const unsigned numOutputs = 1+(unsigned)spin;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new NavigationCortex
   * @param species The root of the species data to read from
   * @param s The Ship to operate on
   * @param ss The SelfSource to use
   */
  NavigationCortex(const libconfig::Setting& species, Ship* s, cortex_input::SelfSource* ss);

  /** Controls the ship given elapsed time and the current objective. */
  void evaluate(float, const GameObject*);

  /** Returns the score of the cortex */
  float getScore() const { return score; }
};

#endif /* C_NAVIGATION_HXX_ */
