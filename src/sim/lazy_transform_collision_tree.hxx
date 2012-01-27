/**
 * @file
 * @author Jason Lingle
 * @brief Contains the LazyTransformCollisionTree class
 */

/*
 * lazy_transform_collision_tree.hxx
 *
 *  Created on: 15.06.2011
 *      Author: jason
 */

#ifndef LAZY_TRANSFORM_COLLISION_TREE_HXX_
#define LAZY_TRANSFORM_COLLISION_TREE_HXX_

#include <vector>

#include "src/opto_flags.hxx"

struct CollisionRectangle;

/** The LazyTransformCollisionTree is an interface for automatically
 * generating a dynamic tree of CollisionRectangles that perform
 * transformations of a base set of CollisionRectangles based on
 * an offset and rotation, as needed. These are arranged in a
 * dynamic binary tree, allowing far faster collision detection
 * and accumulation.
 * This class only works properly if the source quads do not
 * overlap.
 *
 * It should be noted that this structure will often result in
 * duplicates when accumulating, as a Cell can exist in up to
 * four divisions.
 *
 * Manipulations to the secondary parms of the source CollisionRectangle*s
 * are copied once on transformation; there will therefore be a 1-frame
 * delay for such changes to be applied.
 */
class LazyTransformCollisionTree {
  //The internal CollisionRectangle*s for mapping
  CollisionRectangle* internalSource;
  //Externally-provided source rectangles
  const CollisionRectangle*const* externalSource;
  //Maps indices within active to their respective in source
  const CollisionRectangle** sourceMapping;
  CollisionRectangle* active;
  //Return values, pointing to offsets in active
  CollisionRectangle** activeIndirect;
  struct Level {
    LazyTransformCollisionTree* that;
    unsigned offset, len;
  }* levels;
  std::vector<CollisionRectangle*> top;

  float xoff, yoff, cost, sint;

  public:
  /** Constructs a new LazyTransformCollisionTree */
  LazyTransformCollisionTree();
  ~LazyTransformCollisionTree();

  /** Invalidates cached transforms and sets new coordinate information.
   *
   * @param x Centre X coordinate
   * @param y Centre Y coordinate
   * @param ct Result of cos(theta)
   * @param st Result of sin(theta)
   */
  void reset(float x, float y, float ct, float st) noth;

  /** Sets a new set of rectangles. This destroys all current data and
   * rebuilds the tree.
   *
   * The source data is NOT copied.
   */
  void setData(const CollisionRectangle*const*, unsigned len) noth;

  /** Frees all data. */
  void free() noth;

  /** Returns whether there currently is data. */
  bool hasData() const noth { return (bool)active; }

  /** Returns the top of the tree. */
  const std::vector<CollisionRectangle*>& get() const noth { return top; }

  private:
  void transform(CollisionRectangle& dst, const CollisionRectangle& src) noth;

  static void thunkTransform(CollisionRectangle*, CollisionRectangle*const*&, unsigned&);
  static void thunkCached   (CollisionRectangle*, CollisionRectangle*const*&, unsigned&);
};

#endif /* LAZY_TRANSFORM_COLLISION_TREE_HXX_ */
