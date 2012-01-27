/**
 * @file
 * @author Jason Lingle
 * @brief Contains the TargetAnalysisCortex
 */

/*
 * c_target_analysis.hxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#ifndef C_TARGET_ANALYSIS_HXX_
#define C_TARGET_ANALYSIS_HXX_

#include "cortex.hxx"
#include "ci_cellex.hxx"
#include "ci_nil.hxx"

class Ship;
class Cell;

/**
 * The TargetAnalysisCortex analyses randomly-selected Cells in
 * a target ship to determine which Cell to target.
 */
class TargetAnalysisCortex: public Cortex, private cortex_input::CellEx<cortex_input::Nil> {
  Ship*const ship;

  //The most recent Ship examined.
  //This is only used for equality testing, and may be a dangling
  //pointer
  Ship* previousShip;

  //The current best score (decaying)
  float previousBestScore;

  enum Output { base=0, power, capac, engine, weapon, shield };
  static const unsigned numOutputs = 1+(unsigned)shield;

  public:
  typedef cip_input_map input_map;

  /**
   * Constructs a new TargetAnalysisCortex
   * @param species The root of the species to read from
   * @param s The ship to operate on
   */
  TargetAnalysisCortex(const libconfig::Setting& species, Ship* s);

  /**
   * Examines a Cell from the given Ship.
   * Returns a Cell* if it should be targetted instead of the current,
   * or NULL if no change should be made.
   */
  Cell* evaluate(float, Ship*);

  /** Returns the score of the cortex. */
  float getScore() const;
};


#endif /* C_TARGET_ANALYSIS_HXX_ */
