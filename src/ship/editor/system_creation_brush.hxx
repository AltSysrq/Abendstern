/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SystemCreationBrush mode
 */

/*
 * system_creation_brush.hxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#ifndef SYSTEM_CREATION_BRUSH_HXX_
#define SYSTEM_CREATION_BRUSH_HXX_

#include <set>

#include "abstract_system_painter.hxx"

class Cell;

/** The SystemCreationBrush is a system painter that paints cells
 * when there is pressed motion over them. For performance and sanity reasons, it will
 * only paint a cell once per stroke.
 */
class SystemCreationBrush: public AbstractSystemPainter {
  std::set<Cell*> paintedCells;

  public:
  /** Standard mode constructor */
  SystemCreationBrush(Manipulator*,Ship*const&);
  virtual void motion(float,float,float,float);
  virtual void press(float,float);
  virtual void release(float,float);
  virtual void activate();
};

#endif /* SYSTEM_CREATION_BRUSH_HXX_ */
