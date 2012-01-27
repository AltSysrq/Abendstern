/**
 * @file
 * @author Jason Lingle
 * @brief Contains the RightTCell class
 */

#ifndef RIGHTT_CELL_HXX_
#define RIGHTT_CELL_HXX_

#include "cell.hxx"

/** Cell which is a right-isosceles triangle, having three neighbours
 * and holding one system.
 * Side 1 is the hypoteneuse.
 */
class RightTCell : public Cell {
  //Used internally by draw
  bool hasCalculatedColourInfo;
  unsigned ixIx;

  public:
  /** Constructs a RightTCell for the given Ship. */
  RightTCell(Ship* parent);
  virtual unsigned numNeighbours() const noth;
  virtual float edgeD(int) const noth;
  virtual int edgeT(int) const noth;
  virtual int edgeDT(int) const noth;
  virtual float getCentreX() const noth;
  virtual float getCentreY() const noth;
  virtual CollisionRectangle* getCollisionBounds() noth;
  virtual float getIntrinsicDamage() const noth;

  protected:
  virtual void drawThis() noth;
  virtual void drawShapeThis(float,float,float,float) noth;
  virtual void drawDamageThis() noth;
};

#endif /*RIGHTT_CELL_HXX_*/
