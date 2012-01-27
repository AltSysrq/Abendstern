/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CircleCell class.
 */

#ifndef CIRCLE_CELL_HXX_
#define CIRCLE_CELL_HXX_

#include "square_cell.hxx"

/** A CircleCell is really a SquareCell in disguise.
 * It has different intrinsic mass and damage, but is otherwise
 * the same (other than graphical representation).
 */
class CircleCell : public SquareCell {
  public:
  /** Creates a new CircleCell on the given Ship. */
  CircleCell(Ship* s) : SquareCell(s) { _intrinsicMass=65; }
  virtual float getIntrinsicDamage() const noth;

  protected:
  virtual void drawThis() noth;
};

#endif /*CIRCLE_CELL_HXX_*/
