/**
 * @file
 * @author Jason Lingle
 * @brief Contains the FrontalCortex
 */

/*
 * c_frontal.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_FRONTAL_HXX_
#define C_FRONTAL_HXX_

#include "src/sim/objdl.hxx"
#include "cortex.hxx"
#include "ci_nil.hxx"
#include "ci_self.hxx"
#include "ci_objective.hxx"

class GameObject;
class Ship;

/**
 * The FrontalCortex examines possible objectives and returns the one
 * given the highest score.
 *
 * All FrontalCortices maintain a global telepathy map of
 * (insignia,objective) --> cortices...
 */
class FrontalCortex: public Cortex, private
 cortex_input::Self
<cortex_input::Objective
<cortex_input::Nil> > {
  Ship*const ship;
  ObjDL objective;
  float objectiveScore; //Telepathy output
  const float distanceParm, scoreWeightParm, dislikeWeightParm, happyWeightParm;

  //The last known insignia of the ship
  //(Specifically, the insignia when we inserted ourselves
  // into the global map).
  //0 indicates not inserted
  unsigned long insertedInsignia;
  //The GameObject* used to insert us into the global map.
  //This may be a dangling pointer, never dereference
  GameObject* insertedObjective;

  //Milliseconds until the next scan
  float timeUntilRescan;

  //Add the opriority, ocurr, and otel inputs
  static const unsigned cipe_last = cip_last+3,
                        cip_opriority = cip_last,
                        cip_ocurr = cip_last+1,
                        cip_otel = cip_last+2;

  enum Output { target = 0 };
  static const unsigned numOutputs = 1+(unsigned)target;

  public:
  class input_map: public cip_input_map {
    public:
    input_map() {
      ins("opriority", cip_opriority);
      ins("ocurr", cip_ocurr);
      ins("otel", cip_otel);
    }
  };

  /** Gives instructions on how to proceed */
  struct Directive {
    /** The objective to persue.
     * If NULL, park.
     */
    GameObject* objective;
    /** What to do to the objective. */
    enum Mode {
      Attack, /// Proceed with the attack series of cortices
      Navigate /// Phoceed with the Navigation cortex
    } mode;
  };

  /**
   * Constructs a new SelfSource.
   * @param species The root of the species data to read from
   * @param s The Ship to operate on
   * @param ss The SelfSource to use
   */
  FrontalCortex(const libconfig::Setting& species, Ship* s, cortex_input::SelfSource* ss);
  ~FrontalCortex();

  /**
   * Evaluates the cortex and returns its decision, given te elapsed time.
   */
  Directive evaluate(float);

  /** Returns the score of the cortex. */
  float getScore() const;

  private:
  //Remove from global multimap if inserted
  void unmap();
  //Insert to global multimap;
  //if inserted, unmap first.
  //Does nothing if no objective.
  void mapins();
};

#endif /* C_FRONTAL_HXX_ */
