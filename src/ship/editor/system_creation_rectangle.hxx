/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SystemCreationRectangle mode
 */

/*
 * system_creation_rectangle.hxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#ifndef SYSTEM_CREATION_RECTANGLE_HXX_
#define SYSTEM_CREATION_RECTANGLE_HXX_

#include <vector>

#include "abstract_system_painter.hxx"

class Cell;

/**
 * Mode for filling a rectangle with systems.
 *
 * The SystemCreationRectangle allows the user to define a rectangle
 * by dragging the mouse; when released, any cells that have a
 * collision vertex within the rectangle are painted when the
 * mouse is released.
 */
class SystemCreationRectangle: public AbstractSystemPainter {
  /* All cells that the current rectangle affects */
  std::vector<Cell*> targets;

  /* The location where the user first pressed the mouse. */
  float ox, oy;
  /* The current location of the rectangle */
  float cx, cy;

  /* Updates targets according to ox, oy, cx, and cy. */
  void updateTargets();

  public:
  /** Standard mode constructor */
  SystemCreationRectangle(Manipulator*, Ship*const&);

  virtual void activate();
  virtual void press(float,float);
  virtual void motion(float,float,float,float);
  virtual void release(float,float);
  virtual void draw();
};

#endif /* SYSTEM_CREATION_RECTANGLE_HXX_ */
