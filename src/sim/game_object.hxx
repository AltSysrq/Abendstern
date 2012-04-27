/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GameObject class
 */

#ifndef GAME_OBJECT_HXX_
#define GAME_OBJECT_HXX_

//We don't use GL, but must include it for MSVC++ to be happy
#include <GL/gl.h>
#include <SDL.h>

#include <utility>
#include <vector>
#include <string>

#include "collision.hxx"
#include "src/core/aobject.hxx"
#include "src/opto_flags.hxx"
using std::pair;
using std::make_pair;
using std::vector;

/** This macro will automatically ensure that x and y are within
 * the field for remote objects.
 * Even if it is the only statement in a block, it MUST
 * have braces, since it doesn't define its own.
 */
#define REMOTE_XYCK \
  if (x<0) x=0; \
  else if (x>=field->width) x=field->width-0.0001f; \
  if (y<0) y=0; \
  else if (y>=field->height) y=field->height-0.0001f

class GameField;
struct ObjDL;

struct NebulaResistanceElement;

/** A GameObject is exactly what it sounds like: An object in the playing
 * field. This class encompasses basic things such as location, velocity,
 * and interfaces for collision, et cetera.
 * Examples of GameObjects include ships, asteroids, and energy charges.
 */
class GameObject : public AObject {
  friend class GameField;
  friend class TclGameObject;

  public:
  /** If the GameObject is remote, updates only continue extrapolation, and must
   * not cause the object to cease to exist (most importantly, they must go out
   * of their way to ensure the object stays within the boundaries).
   */
  bool isRemote;
  /** Whether the GameObject is a candidate for network exportation.
   * Defaults to false.
   */
  bool isExportable;
  /** Whether the GameObject is considered "transient" on the network.
   * A transient object is considered short-ranged and short-lived, and
   * is only exported to remote peers if they are close enough.
   * Defaults to true.
   */
  bool isTransient;
  /**
   * If true, this object will be unaffected by calles to GameField::clear()
   * regardless of whether it is remote.
   *
   * By default, false.
   */
  bool skipOnClear;

  /** Keeps track of the head listener. */
  ObjDL* listeners;

  /** Objects may be given a human-readable textual tag.
   * If ignoreNetworkTag is set to true,
   * requests from the network to change the tag will be ignored
   * silently.
   */
  std::string tag;
  bool ignoreNetworkTag;

  /** Information for interaction with nebulae.
   * If nebulaInteraction is true, the object will be considered by any nebula
   * background. The nebulaFriction variables is a force (screen*mass/ms/ms)
   * applied to the object linearly; nebulaTorsion indicates a torque
   * (screen*screen*mass/ms/ms) applied about the object's centre of mass.
   * These are set by the Nebula class and read by the subclass.
   * nebulaCache is set to true if the cached properties are valid. The
   * object should set this variable to false whenever a meaningful physical
   * change occurs. The meaning of the cache variables is internal to the
   * Nebula class and should not be modified by anyone else.
   */
  bool nebulaInteraction, nebulaCache; ///< @see nebulaInteraction
  float nebulaFrictionX, ///< @see nebulaInteraction
        nebulaFrictionY, ///< @see nebulaInteraction
        nebulaTorsion; ///< @see nebulaInteraction
  float nebulaCacheFFrict, ///< @see nebulaInteraction
        nebulaCacheSFrict; ///< @see nebulaInteraction

  protected:
  /** The GameField that owns this GameObject.
   */
  GameField*const field;

  /** Generally returned from getCollisionBounds() */
  std::vector<CollisionRectangle*> collisionBounds;

  /* Locations and speeds are based on screen width, where each whole
   * number is one screen wide. Velocity is in widths per millisecond.
   */
  float x, ///< The X coordinate of the object, in screen-widths
        y, ///< The Y coordinate of the object, in screen-widths
        vx,///< The X velocity of the object, in screen-widths per millisecond
        vy;///< The Y velocity of the object, in screen-widths per millisecond

  /**
   * Indicates whether the object is included in collision detection.
   *
   * Most objects should be included in collision detection. Purely decorative
   * objects (like plasma fires) and those with special handling (Shields) are
   * not included. If this is set to false (default true), the object will not
   * be added to the sectors arrays.
   */
  bool includeInCollisionDetection;

  public:
  /** Allows the AI to quickly determine what kind of GameObject
   * this is.
   */
  enum Classification { Generic, ClassShip, LightWeapon, HeavyWeapon };
  /** Defines the draw layer of the object.
   * This enum is in back-to-front order.
   */
  enum DrawLayer { Background=0, Foreground, Overlay };
  protected:
  Classification classification; ///< The Classification of this object
  /** DrawLayer defaults to Foreground, which is appropriate for
   * most objects.
   */
  DrawLayer drawLayer;

  /** Indicates the decorative status of the object on the next frame.
   *
   * Some GameObjects are more decorative than anything else, such as
   * floating ship pieces. These are updated on a physical frame basis,
   * to reduce the load on the more critical objects.
   */
  bool decorative;

  private:
  /** The real state of "decorative".
   *
   * Why this is necessary takes some explanation.
   * In most cases, if the state of decorative changes mid-physical-frame, the object
   * is merely doubly-updated, which is OK. The problem occurs if it changes on the
   * last virtual frame before the physically-timed update. Since the sectors are not
   * cleared for the PT update (to save time), the object will end up in sectors twice.
   * Then, collision-detection will pass through, rightfully detect that the object overlaps
   * itself, and call the collideWith function on it twice. Classes should return false
   * when colliding with themselves (normally indicating immediately-impending deletion), so
   * the collision handler deletes it, removes it from the field, and nullifies it. Then
   * it tries to delete the second one. The virtual destructor runs, which (for a ship) tries
   * to delete its cells, ultimately landing in an almost random location of memory. Thus,
   * the program crashes HARD.
   *
   * Only decorative is directly available to subclasses. isDecorative() returns this value.
   * After each physical frame, the field calls okToDecorate(), which copies decorative into
   * this field.
   */
  bool reallyDecorative;

  /** The following variables are used for collision detection.
   * First, a history of how we did collision detection:
   * Originally, we just checked every object against every other, requiring n*(n-1)/2 checks
   * per frame. When this became a problem, a new grid system was employed, in which objects
   * were dropped into 1x1-screen buckets, and only checked against other objects in the same
   * bucket and those in the immidiately-right, -top, and -top-right. While this was much
   * faster, it required ridiculous amounts of memory (sizeof(void*)*width*height*1025) and
   * limitted objects to being one screen in radius. This grid system was used until 11 July 2010.
   *
   * The next system works by arranging objects into a sorted binary tree, keyed by the right
   * bound (x+radius). Once this is done, we pass through the objects and construct them into
   * a reverse-sorted singly-linked list.
   * To check an object for collisions, we begin scanning through the list. For each object,
   * we do an actual collision check if and only if the vertical bounds overlap. We stop
   * searching when we reach an object whose right bound is less than our left bound.
   *
   * 2011.02.20:
   * Building the tree takes too long for >1000 objects, which is a serious issue with the
   * gatling plasma burst launcher. However, we can take advantage of the fact that objects
   * do not move THAT much in relation to each other per frame. If we keep the deque of
   * objects sorted, any order changes can be corrected quickly with gnome sort. We then
   * use the previous system for checking for collisions. New objects must be inserted by
   * a binary-search, and only so when not iterating through the objects.
   */
  struct {
    /** In case an object dies during collision application, we need to know it can't be
     * checked against anything else.
     */
    bool isDead;
    /* Various bounds */
    float upper, ///< High Y bound of bounding box
          lower, ///< Low Y bound of bounding box
          left,  ///< Low X bound of bounding box
          right; ///< High X bound of bounding box
  } ci; ///< Collision detection information

  protected:
  /**
   * Creates a new GamebOject.
   * @param _field The GameField the object lives in
   * @param _x Initial X coordinate
   * @param _y Initial Y coordinate
   * @param _vx Initial X velocity
   * @param _vy Initial Y velocity
   */
  GameObject(GameField* _field, float _x=0.0f, float _y=0.0f,
             float _vx=0.0f, float _vy=0.0f)
  : isRemote(false), isExportable(false), isTransient(true),
    skipOnClear(false),
    listeners(NULL),
    ignoreNetworkTag(false),
    nebulaInteraction(false), nebulaCache(false),
    nebulaFrictionX(0), nebulaFrictionY(0), nebulaTorsion(0),
    field(_field), x(_x), y(_y), vx(_vx), vy(_vy),
    includeInCollisionDetection(true),
    classification(Generic), drawLayer(Foreground),
    decorative(false), reallyDecorative(false)
  {
    ci.isDead=false;
  }

  public:
  /** Updates a single frame by the given number of milliseconds.
   * @param elapsedTime The elapsed time, in milliseconds, since the last call to update()
   * @return True if the object should still exist, false otherwise
   */
  virtual bool update(float elapsedTime) noth = 0;

  /** Draws the object.
   *
   * There is no guarantee of a 1:1 ratio with
   * update(float). In fact, at lower framerates, update(float) will
   * be called much more frequently than draw().
   * This function will not be called if, based on coordinates and radius,
   * it appears that this object will not show on screen.
   */
  virtual void draw() noth = 0;

  /** Checks for a collision with the given GameObject.
   * This is called at sometime after update(float) but before
   * draw().
   * @see src/sim/collision.hxx
   * @see CollisionResult
   * @see objectsCollide()
   */
  virtual CollisionResult checkCollision(GameObject* other) noth {
    return UnlikelyCollision;
  }

  /** Perform the apropriate action for collision with an object.
   * By default, do nothing.
   * Return whether the object should still exist.
   * If itself is passed, it is being destroyed for non-physical
   * reasons, and the value returned is meaningless.
   * Collisions may also occur even when there is no real "collision",
   * such as a proximity explosion. GameObjects should know how to
   * properly handle these.
   */
  virtual bool collideWith(GameObject* other) noth {
    return other!=this;
  }

  /* Return the respective variables.
   * Notice that these are not virtual.
   */
  inline float getX() const { return x; } ///< Returns the X coordinate
  inline float getY() const { return y; } ///< Returns the Y coordinate
  inline float getVX() const { return vx; } ///< Returns the X velocity
  inline float getVY() const { return vy; } ///< Returns the Y velocity
  inline GameField* getField() const { return field; } ///< Returns the containing GameField
  inline bool isDecorative() const { return reallyDecorative; } ///Returns the decorative status
  inline Classification getClassification() const { return classification; } ///Returns the classification

  /** Propagates the decorative value to reallyDecorative.
   *
   * This should generally only be called by GameField, but is public since
   * there are occasionally other reasons to call it.
   */
  void okToDecorate() noth { reallyDecorative=decorative; }

  /** Teleport the object to the given coordinates and rotation.
   * The default implementation just sets x and y and ignores theta.
   *
   * @param x X coordinate to move to.
   * @param y Y coordinate to move to.
   * @param theta Rotation to set to. Ignored by default.
   */
  virtual void teleport(float x, float y, float theta) noth {
    this->x=x;
    this->y=y;
  }

  /** Return the rotation of the object in radians,
   * if there is such a thing. Otherwise,
   * just return 0.
   */
  virtual float getRotation() const noth { return 0; }

  /** Returns the "radius" of the object.
   * That is, the radius for a circle with the same centre as this
   * object where the circle exactly touches, but never intersects,
   * the graphical representation of this object.
   */
  virtual float getRadius() const noth = 0;

  /** Returns a vector of collision rectangles.
   * The default just returns an empty rectangle.
   * The caller should not delete the vector when finished,
   * nor the pointers within.
   */
  virtual const std::vector<CollisionRectangle*>* getCollisionBounds() noth;

  /** Returns whether the object is physically collideable.
   * Default returns false.
   */
  virtual bool isCollideable() const noth { return false; }

  /** Returns the NebulaResistenceElements associated with this object.
   * Default returns NULL; this MUST be overridden if nebulaInteraction.
   * The returned item is NOT to be deleted by the caller.
   * The pointer is interpreted as an array whose length is given by
   * the next function.
   */
  virtual const NebulaResistanceElement* getNebulaResistanceElements() noth { return NULL; }
  /** Returns the length of the array returned by getNebulaResistanceElements.
   * This will always called immediately after getNebulaResistanceElements.
   * Default returns 0.
   */
  virtual unsigned getNumNebulaResistanceElements() const noth { return 0; }

  /** Enqueue the GameObject for deletion at the end of
   * the next physical frame. This should be used instead of
   * operator delete.
   *
   * This does NOT remove it
   * from the GameField if it is still in there.
   */
  void del() noth;

  virtual ~GameObject();

private:
  void deleteCommon() noth;
};

#endif /*GAME_OBJECT_HXX_*/
