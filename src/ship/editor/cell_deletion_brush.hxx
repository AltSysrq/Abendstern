/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellDeletionBrush mode
 */

/*
 * cell_deletion_brush.hxx
 *
 *  Created on: 06.02.2011
 *      Author: jason
 */

#ifndef CELL_DELETION_BRUSH_HXX_
#define CELL_DELETION_BRUSH_HXX_

#include "manipulator_mode.hxx"

/** The CellDeletionBrush deletes any cell the user clicks on. Any orphans
 * are also deleted. Unlike most brushes, it only responds to the initial
 * click, to reduce the chance of serious accidents.
 */
class CellDeletionBrush: public ManipulatorMode {
  public:
  /** Standard mode constructor */
  CellDeletionBrush(Manipulator*,Ship*const&);
  virtual void press(float,float);
  virtual void activate();
};

#endif /* CELL_DELETION_BRUSH_HXX_ */
