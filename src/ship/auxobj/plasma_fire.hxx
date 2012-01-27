/**
 * @file
 * @author Jason Lingle
 * @brief Contains the PlasmaFire class
 */

#ifndef PLASMA_FIRE_HXX_
#define PLASMA_FIRE_HXX_

#include <vector>

#include "src/ship/cell/empty_cell.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/collision.hxx"
#include "src/opto_flags.hxx"

/** A PlasmaFire is a visual effect used only in high-quality mode
 * in which fire is emitted by newly broken-off pieces of ship.
 * It is fixed to an EmptyCell.
 */
class PlasmaFire : public GameObject {
  friend class EmptyCell;
  private:
  //Our fix. If it is deleted, this is
  //set to NULL, so we know to die
  EmptyCell* fix;

  //The angle of emission relative to ship
  float angle;

  float timeLeft;
  float timeSinceLastExplosion;

  public:
  /** Constructs a new PlasmaFire emitted from the given EmptyCell.
   *
   * @param f The cell from where the fire comes.
   */
  PlasmaFire(EmptyCell* f);
  virtual ~PlasmaFire();
  virtual bool update(float) noth;
  virtual void draw() noth {}

  virtual bool isCollideable() noth { return false; }
  virtual float getRadius() const noth { return 0.010f; }
  virtual CollisionResult checkCollision(GameObject*) noth { return NoCollision; }
};

#endif /*PLASMA_FIRE_HXX_*/
