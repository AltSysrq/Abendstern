/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GameField class
 */

#ifndef GAME_FIELD_HXX_
#define GAME_FIELD_HXX_

//We don't use GL, but include so MSVC++ doesn't complain
#include <GL/gl.h>
#include <SDL.h>

#include <vector>
#include <deque>
#include <queue>

#include "../core/aobject.hxx"
#include "../background/explosion_pool.hxx"
#include "../opto_flags.hxx"

//Circluar dependancy, so just predeclare in the header
class GameObject;
class Explosion;
class Blast;
class EffectsHandler;
class NetworkAssembly;

/** The GameField handles the managing, updating, and drawing of GameObjects.
 *
 * If too much time elapses between frames, it breaks the frame down into
 * smaller updates, to allow for accurate collision detection.
 */
class GameField : public AObject {
  private:
  std::deque<GameObject*> objects;

  bool isInCollisionSubcycle, isUpdatingDecorative;

  std::deque<GameObject*> toInject;

  //We do need a VAO/VBO pair for drawing the boundaries.
  GLuint vao, vbo;

  public:
  /** Although this is public, other classes should not access
   * its add functions.
   *
   * Convention is to pass all explosions
   * through GameField::add.
   */
  ExplosionPool expool;

  /** This is similar to the global gameClock, but keeps time
   * relative to the field, in milliseconds.
   * It starts at zero.
   * Code should not need to prepare for wrap-around, as that
   * will take 49 days.
   *
   * Other code should not modify this.
   */
  Uint32 fieldClock;

  private:
  /* Make sure we have proper support for clock units shorter
   * than 1 ms, even though the SDL method is only granular
   * to 1 ms.
   */
  float fieldClockMicro;

  //Instead of doing instant additions to the beginning,
  //we store them in here until the end of the frame.
  //This allows us to use local variables for better
  //performance than the previous volatile instance
  //variables
  std::deque<GameObject*> toInsert;

  int nw, nh;

  std::vector<GameObject*> toDelete;

  public:
  /** The maximum width for the field.
   * The field extends from 0..width, excluding width itsellf.
   * If any object goes outside, it will be destroyed.
   */
  float width, height; ///< The maximum height of the field. @see width

  /** A set of GameObjects that are to be deleted at the
   * end of the next physical frame. We do this so that all ObjRefs have
   * a chance to catch the deletion before something else
   * is allocated at the same location (if we did delete them
   * immediately, on rare occasions, a new object would be
   * created at the same address).
   *
   * As ObjRef has been removed, this is no longer necessary for C++,
   * but Tcl needs to poll objects to see if they died, so the delay
   * is still a good idea.
   */
  std::vector<GameObject*> deleteNextFrame;

  /**
   * The current EffectsHandler
   */
  EffectsHandler* effects;

  /**
   * If true, all ships are visible on radar regardless of range/stealth.
   * Defaults to false.
   */
  bool perfectRadar;

  /**
   * The NetworkAssembly to notify about object creations and deletions.
   *
   * Initialised to NULL.
   */
  NetworkAssembly* networkAssembly;

  /** Constructs a new GameField with the given size.
   *
   * @param width Width of the field
   * @param height Height of the field
   */
  GameField(float width, float height);
  virtual ~GameField();
  /** Updates the GameField and all GameObjects contained.
   *
   * @param et Elapsed time, in milliseconds, since the previous call to update()
   */
  void update(float et);
  /** Draws the GameField and all GameObjects contained. */
  void draw();

  //Access a particular GameObject
  /** Returns the GameObject* at the given index. No bounds checking is performed. */
  inline GameObject* operator[](unsigned at) const noth { return objects[at]; }
  /** Returns the GameObject* at the given index. No bounds checking is performed. */
  inline GameObject* at        (unsigned at) const noth { return objects[at]; }
  /** Return the number of GameObjects in the field */
  unsigned size() const noth { return objects.size(); }
  /** Adds a new GameObject to the field */
  void add(GameObject*) noth;
  /** Adds a new GameObject at the beginning of the next frame */
  void addBegin(GameObject*) noth;
  /** Removes an object from the field.
   * This function does NOT delete the object.
   */
  void remove(GameObject*) noth;
  /**
   * Removes the object from the current insert queue, if present.
   * Has no effect if not in the insert queue.
   */
  void removeFromInsertQueue(GameObject*) noth;

  /** Adds an Explosion* to the ExplosionPool.
   * The Explosion will be deleted.
   * This function is no different from addBegin().
   */
  void add(Explosion*) noth;
  /** Adds an Explosion* to the ExplosionPool.
   * The Explosion will be deleted.
   * This function is no different from add().
   */
  void addBegin(Explosion*) noth;

  /**
   * Removes and deletes all non-remote objects in the GameField.
   */
  void clear() noth;

  typedef std::deque<GameObject*>::const_iterator iterator;
  inline iterator begin() const noth { return objects.begin(); }
  inline iterator end()   const noth { return objects.end(); }

  /**
   * Adds a GameObject* as soon as it is possible to do
   * so without interfering with collision detection.
   *
   * Certain objects, such as Blasts, MUST experience
   * the update-collide cycle the same frame as added.
   * This causes out-of-date issues when added in the
   * collision cycle. An object sensitive to this should
   * be inserted with this function by the creator if
   * there is ever the possibility of that code running
   * during the collision subcycle.
   *
   * It has the same effect as add(GameObject*) when not
   * in the collision subcycle; otherwise, calls go->update(float)
   * and /manually/ checks the object for collision with others
   * in appropriate sectors.
   *
   * The actual work will not run until after all standard collision
   * detection has completed.
   */
  void inject(GameObject*) noth;

  private:
  template<bool decor>
  void updateImpl(float) noth HOTFLAT;
  //Split up for profiling
  #ifdef PROFILE
  template<bool decor>
  void updateImpl_update(float) noth HOT;
  template<bool decor>
  void updateImpl_collision(float) noth HOT;
  template<bool decor>
  void updateImpl_begin(float) noth;
  #endif

  void doInject(GameObject*, float subTime) noth;
  unsigned doAdd(GameObject*) noth;
};

#endif /*GAME_FIELD_HXX_*/
