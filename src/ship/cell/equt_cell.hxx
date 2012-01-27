/**
 * @file
 * @author Jason Lingle
 * @brief Contains the EquTCell class
 */

#ifndef EQUT_CELL_HXX_
#define EQUT_CELL_HXX_

#include "cell.hxx"

/**
 * An equilateral-triangle cell, which has three neighbours and one system.
 * Side 1 with no rotation is vertical, on the left.
 */
class EquTCell : public Cell {
  /* Used internally by draw() */
  bool hasCalculatedColourInfo;
  unsigned ixIx;
  public:
  /** Constructs a new EquTCell for the given Ship. */
  EquTCell(Ship* parent);
  virtual unsigned numNeighbours() const noth;
  virtual float edgeD(int) const noth;
  virtual int edgeT(int) const noth;
  virtual int edgeDT(int) const noth;
  virtual CollisionRectangle* getCollisionBounds() noth;
  virtual float getIntrinsicDamage() const noth;

  protected:
  virtual void drawThis() noth;
  virtual void drawShapeThis(float,float,float,float) noth;
  virtual void drawDamageThis() noth;
};

#endif /*EQUT_CELL_HXX_*/
