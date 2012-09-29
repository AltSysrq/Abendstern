/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Shield class.
 * @see src/ship/sys/b/shield_generator.hxx
 */

#ifndef SHIELD_HXX_
#define SHIELD_HXX_

#include "src/sim/collision.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"
#include "src/core/aobject.hxx"

//The ShieldGenerator class does not check these;
//they must be checked by the creator
#define MIN_SHIELD_RAD 2
#define MIN_SHIELD_STR 5
#define MAX_SHIELD_RAD 15
#define MAX_SHIELD_STR 50

/** A Shield defines a circular area that protects everything
 * contained within.
 *
 * This protection is that weapons are stopped /before/ they actually hit
 * the Ship; it does not actually prevent damage that the Ship would otherwise
 * take.
 *
 * A shield has two values concerning health:
 * strength and stability. For strength to heal, stability must
 * first be 100%. Whenever a shield or one of its protectees is
 * damaged, its stability immediately falls to 0, and the health
 * is impacted as one would expect.
 *
 * The energy required to maintain a shield is directly proportional
 * to the square of its radius and to its total strength. The restabilization
 * speed is inversely proportional to the square of the radius alone, while
 * healing of strength is inversely proportional to the radius (not squared)
 * and to the total strength.
 *
 * This class was originally implemented as another GameObject. However, this
 * incurred a drag on performance, especially in the lines of collision detection
 * and updating of objects (as most ships have more than 1 shield).
 */
class Shield: public AObject {
  friend class ShieldGenerator;
  private:
  float strength, stability;
  //The radius is given in units of STD_CELL_SZ
  float maxStrength, radius, radiusInv, radiusInvSq;
  Cell* parent;

  float xoff, yoff, dist;

  //When the shield is damaged, we draw the shield as a circle
  //with the outer alpha as below
  float alpha;

  #ifndef DEBUG
  #define NUM_RECTS 4
  #else
  #define NUM_RECTS 3
  #endif
  CollisionRectangle collisionRects[NUM_RECTS];
  CollisionRectangle baseRects     [NUM_RECTS];
  CollisionRectangle*collisionRectPtrs[NUM_RECTS];
  #undef NUM_RECTS

  /* Instead of updating collisionRects every cycle, only
   * do it when we are actually needed for collision.
   */
  bool needUpdateRects, needUpdateXYOffs;

  /* Used to convert the old if/else sequence in update(float)
   * into a fast switch statement.
   */
  enum { Full, Heal, Stabilize, Dead } healState;

  void initBases() noth;

  public:
  /** Constructs a Shield with the given parms.
   *
   * @param field The GameField the parent Ship lives in
   * @param par The Cell that contains the generator and to which the Shield is fixed
   * @param str Maximum strength for the Shield
   * @param rad Radius of the Shield
   */
  Shield(GameField* field, Cell* par, float str, float rad)
  : strength(0), stability(1), maxStrength(str),
    radius(rad), radiusInv(1.0f/rad), radiusInvSq(1.0f/rad/rad),
    parent(par),
    alpha(0), needUpdateRects(true), needUpdateXYOffs(true),
    healState(Heal)
  {
    for (unsigned i=0; i<lenof(collisionRects); ++i) {
      collisionRects[i].radius=STD_CELL_SZ*rad*2;
      collisionRects[i].data = NULL; //Because Ship uses these to identify
                                     //cells
      collisionRectPtrs[i] = &collisionRects[i];
    }

    initBases();
  }

  /** Performs necessary updates.
   * @param et Time elapsed, in milliseconds, since the last call to update()
   */
  void update(float et) noth HOT;
  /** Updates the dist member */
  void updateDist() noth;
  /** Returns the distance from the centre of the containing ship to the Shield. */
  float getDistance() const noth { return dist; }
  /** Draws the Shield */
  void draw() noth;
  /** Returns the radius of the Shield. */
  float getRadius() const noth;
  /** Handles collision.
   * @see GameObject::collideWith()
   */
  bool collideWith(AObject*) noth;

  /** Returns the current strength of the Shield. */
  float getStrength() const noth { return strength; }
  /** Returns the current stability of the Shield. */
  float getStability() const noth { return stability; }
  /** Returns the maximum strength of the Shield. */
  float getMaxStrength() const noth { return maxStrength; }

  /** Returns the Cell that contains the Shield */
  Cell* getParent() const noth { return parent; }

  private:
  void updateXYOffs() noth;

  void setStrength(float s) noth { maxStrength=s; }
  void setRadius(float r) noth {
    radius=r;
    radiusInv=1.0f/r;
    radiusInvSq=radiusInv*radiusInv;
    initBases();
  }

  public:
  /** Adds the Shield's collision boundaries to the given vector. */
  void addCollisionBounds(vector<CollisionRectangle*>&) noth;

  /** Returns the Ship this Shield belongs to. */
  Ship* getShip() noth { return parent? parent->parent : NULL; }

  /** Draws a representation of the shield status
   * as per the HUD. The colour is passed in
   * the four arguments, and the view translated
   * and scaled.
   * This is currently unimplemented.
   */
  void drawForHUD(float r, float g, float b, float a) noth;
};

#endif /*SHIELD_HXX_*/
