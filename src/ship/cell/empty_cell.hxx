/**
 * @file
 * @author Jason Lingle
 * @brief Contains the EmptyCell class
 */

#ifndef EMPTY_CELL_HXX_
#define EMPTY_CELL_HXX_

#include "cell.hxx"
#include "src/ship/ship.hxx"

class PlasmaFire;

/** An EmptyCell is simply a placeholder for one that was destroyed, so
 * that the other cells still draw the connections. It has exactly
 * one neighbour.
 *
 * An EmptyCell still /does/ use the normal power.. but that's OK, because
 * a broken-off piece on a ship would create loss.
 */
class EmptyCell : public Cell {
  public:
  PlasmaFire* fire;

  /** Constructs a new EmptyCell.
   *
   * @param ship The Ship that contains the cell; the EmptyCell is not automatically added
   * @param neighbour Initial neighbour, defaulting to NULL
   */
  EmptyCell(Ship* ship, Cell* neighbour=NULL);
  virtual ~EmptyCell();
  virtual unsigned numNeighbours() const noth;
  virtual float edgeD(int n) const noth;
  virtual int edgeT(int n) const noth;
  //Returns NULL
  virtual CollisionRectangle* getCollisionBounds() noth;

  virtual float getIntrinsicDamage() const noth;

  protected:
  virtual void drawThis() noth;
  virtual void drawShapeThis(float,float,float,float) noth;
  virtual void drawDamageThis() noth;
};

#endif /*EMPTY_CELL_HXX_*/
