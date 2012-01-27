/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SquareCell class
 */

#ifndef SQUARE_CELL_HXX_
#define SQUARE_CELL_HXX_

#include <GL/gl.h>

#include "cell.hxx"

/**
 * The SquareCell is a Cell that can hold two systems and has four neighbours.
 * It is also the superclass of CircleCell.
 */
class SquareCell: public Cell {
  /* Internally used by draw() function. */
  bool hasCalculatedColourInfo;
  unsigned ixIx;

  public:
  /** Constructs a SquareCell for the given Ship. */
  SquareCell(Ship* ship);
  virtual unsigned numNeighbours() const noth;
  virtual float edgeD(int) const noth;
  virtual int edgeT(int) const noth;
  virtual CollisionRectangle* getCollisionBounds() noth;
  virtual float getIntrinsicDamage() const noth;

  protected:
  virtual void drawDamageThis() noth;
  virtual void drawThis() noth;
  virtual void drawShapeThis(float,float,float,float) noth;

#ifdef AB_OPENGL_14
  void drawAccessories() noth;
#endif /* AB_OPENGL_14 */

  //These handle most of the actual drawing. Circles need
  //to do some operations between them though, hence why
  //they are split off
  void drawSquareBase() noth;
  void drawSystems() noth;
  //Although it may seem better to put this responsibility
  //in the circle class, it is more efficient to use the
  //after-effects of drawSquareBase() to draw a circle
  //overlay, and having this part of SquareCell will
  //make maintanence easier.
  //This MUST be called immediately after drawSquareBase()
  void drawCircleOverlay() noth;
};

#endif /*SQUARE_CELL_HXX_*/
