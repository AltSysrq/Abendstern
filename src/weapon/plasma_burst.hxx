/**
 * @file
 * @author Jason Lingle
 * @brief Contains the PlasmaBurst weapon
 */

#ifndef PLASMA_BURST_HXX_
#define PLASMA_BURST_HXX_

#include <vector>

#include "src/sim/game_object.hxx"
#include "src/sim/objdl.hxx"
#include "explode_listener.hxx"

class Ship;

#define PLASMA_BURST_RADIUS 0.005f ///< Radius of PlasmaBurst
//Half mass every second
#define PB_DEGREDATION 0.5f ///< Amount to multiply mass by every scond for PlasmaBurst
#define PB_SPEED 0.00125f ///< Launch speed of PlasmaBurst


/** A PlasmaBurst is a very small, unguided projectile that
 * expells mass over time (leaving a trail) and inflicts
 * medium damage in a very concentrated area.
 *
 * The actual projectile in only a few pixels across.
 */
class PlasmaBurst : public GameObject {
  friend class INO_PlasmaBurst;
  friend class ENO_PlasmaBurst;
  friend class ExplodeListener<PlasmaBurst>;

  private:
  ExplodeListener<PlasmaBurst>* explodeListeners;
  
  Ship* parent;
  float mass;
  float direction;
  //Delay arming until 50ms after firing,
  //so that we do not collide with the launcher
  float timeUntilArm;
  /* Don't arm while we are still within our parent's shields. */
  bool inParentsShields, hitParentsShields;
  float timeSinceLastExplosion;

  CollisionRectangle colrect;

  bool exploded;

  //Pointer to our graphical trail
  ObjDL trail;

  unsigned blame;

  //For the net
  PlasmaBurst(GameField*, float x, float y, float vx, float vy, float theta,
              float mass);

  public:
  /**
   * Constructs a PlasmaBurst with the given parms.
   *
   * @param field The field the PlasmaBurst will live in
   * @param par The Ship that launched the PlasmaBurst
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param sourceVX Base X velocity (not including launch speed)
   * @param sourceVY Base Y velocity (ont including launch speed)
   * @param theta Launch direction
   * @param mass Initial mass
   */
  PlasmaBurst(GameField* field, Ship* par, float x, float y, float sourceVX,
              float sourceVY, float theta, float mass);
  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual CollisionResult checkCollision(GameObject*) noth;
  //Default works
  //virtual std::vector<CollisionRectangle*>* getCollisionBounds() noth;
  virtual bool collideWith(GameObject*) noth;
  virtual float getRadius() const noth;
  virtual float getRotation() const noth { return direction; }

  float getMass() const { return mass; } ///<Returns the mass of the PlasmaBurst

  private:
  void explode(GameObject*) noth;
};

#endif /*PLASMA_BURST_HXX_*/
