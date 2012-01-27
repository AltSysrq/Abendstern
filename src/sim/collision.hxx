/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions and definitions for collision detection
 */

#ifndef COLLISION_HXX_
#define COLLISION_HXX_
#include <utility>
#include <vector>

#include "src/opto_flags.hxx"
#include "src/core/aobject.hxx"

class GameObject;

/** To be returned by checkCollision(GameObject*). */
enum CollisionResult {
  /** There was certainly no collision */
  NoCollision,
  /** There probably was not a collision, but
   * check the other object in case. If the
   * other returns anything other than YesCollision,
   * assume no collision.
   */
  UnlikelyCollision,
  /** There may have been a collision, but check
   * the other object to be sure. If the other returns
   * this value or YesCollision, there was a collision;
   * otherwise, there was not.
   */
  MaybeCollision,
  /** There was definitely a collision */
  YesCollision
};

/**
 * Defines a single collision-detection quadralateral.
 *
 * Objects define their boundaries in terms of quadralateralia.
 * The owner field is used to hold data about particular
 * collideables.
 *
 * The radius is to be greater than or equal to the greatest
 * distance any vertex is from the zeroth vertex.
 *
 * If the recurse pointer is non-NULL, it will be called to
 * determine the next level of comparison; otherwise, this
 * rectangle is considered terminal.
 */
struct CollisionRectangle {
  /** The four vertices of this quad */
  std::pair<float,float> vertices[4];
  /** At least the distance from vertex zero to any other. */
  float radius;
  /** Allows a CollisionRectangle to trigger other actions that cause
   * collision detection to recurse.
   *
   * If this is non-NULL, it will be called when a collision occurs
   * involving this CollisionRectangle.
   *
   * @param that The CollisionRectangle that is being operated on
   * @param nxt The function must set this to a CollisionRectangle*const* which is
   *   an array of CollisionRectangle*s to test next
   * @param cnt The function must set this to the length of the array stored into nxt
   */
  void (*recurse)(CollisionRectangle* that, CollisionRectangle*const*& nxt, unsigned& cnt);

  /** Arbitrary data for use by recurse(). */
  void* data;

  CollisionRectangle() : recurse(NULL) { }
};

/** Returns whether two rectangles collide.
 */
bool rectanglesCollide(const CollisionRectangle&, const CollisionRectangle&) noth;

/** Checks whether two objects collide according to their boundaries.
 *
 * It first checks distance against radii, to prevent unnecessary
 * checks.
 *
 * Optional parms may be passed to instruct the function to store
 * the owners of respective CollisionRectangles that collided.
 * The defaults are the two extern pointers above.
 */
bool objectsCollide(GameObject*, GameObject*) noth HOT;

/** Performs the same check as objectsCollide, but accumulates the lowest-level
 * CollisionRectangle*s from the left object that do collide into the given vector.
 */
void accumulateCollision(GameObject*, GameObject*, std::vector<const CollisionRectangle*>&) noth;

/** Returns a point on one of the edges of the quadrillateral that is the closest
 * to the given point.
 */
std::pair<float,float> closestEdgePoint(const CollisionRectangle&, const std::pair<float,float>&) noth;

#endif /*COLLISION_HXX_*/
