/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellRotator mode
 */

/*
 * cell_rotator.hxx
 *
 *  Created on: 08.02.2011
 *      Author: jason
 */

#ifndef CELL_ROTATOR_HXX_
#define CELL_ROTATOR_HXX_

#include "manipulator_mode.hxx"

/** The CellRotator rotates right-triangles counter-clockwise when clicked.
 * The rotation is repeated until the ship reloads successfully.
 * It only makes sense to rotate right-triangles, as all other cell types
 * are radially symmetric.
 */
class CellRotator: public ManipulatorMode {
  public:
  /** Standard mode constructor */
  CellRotator(Manipulator*,Ship*const&);
  virtual void activate();
  virtual void press(float,float);
};

#endif /* CELL_ROTATOR_HXX_ */
