/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Missile weapon
 */

/*
 * missile.hxx
 *
 *  Created on: 07.03.2011
 *      Author: jason
 */

#ifndef MISSILE_HXX_
#define MISSILE_HXX_

#include <vector>

#include "src/sim/game_object.hxx"
#include "src/sim/objdl.hxx"
#include "explode_listener.hxx"

class Ship;

/// The additional speed a Missile gets on launch
#define MISSILE_LAUNCH_SPEED 0.002f

/** The Missile is a fully-guided class A weapon.
 *
 * It constantly accelerates in the direction of its target.
 * It has a finite lifetime, during which guidance decays. After
 * its life is expired, it spontaneously explodes.
 * Guidance is inversely proportional to level.
 * To enhance guidance, the missile experiences constant friction.
 * It will also explode upon collision with its target. It cannot
 * collide with anything else.
 */
class Missile: public GameObject {
  friend class INO_Missile;
  friend class ENO_Missile;
  friend class ExplodeListener<Missile>;

  ExplodeListener<Missile>* explodeListeners;

  ObjDL trail, target, parent;
  int level;
  float timeAlive;
  bool exploded;
  CollisionRectangle colrect;
  std::vector<CollisionRectangle*> collisionBounds;

  unsigned blame;

  //Acceleration to send to remote peers, and to use for tracking when
  //this is a remote object.
  float ax, ay;

  //Networking constructor
  Missile(GameField*, int, float, float, float, float, float, float,
          float timeAlive);

  public:
  /** Constructs a Missile with the given parms.
   *
   * @param field The field in which the Missile lives
   * @param level Energy level
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param vx Initial X velocity
   * @param vy Initial Y velocity
   * @param par Ship that launched the Missile
   * @param tgt Object to guide to and collide with
   */
  Missile(GameField* field, int level, float x, float y, float vx, float vy,
          Ship* par, GameObject* tgt);
  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual float getRadius() const noth;
  virtual CollisionResult checkCollision(GameObject*) noth;
  virtual bool collideWith(GameObject*) noth;
  virtual const std::vector<CollisionRectangle*>* getCollisionBounds() noth;

  private:
  void explode(GameObject*) noth;
};

#endif /* MISSILE_HXX_ */
