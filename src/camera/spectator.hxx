/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Spectator object
 */

/*
 * spectator.hxx
 *
 *  Created on: 08.10.2011
 *      Author: jason
 */

#ifndef SPECTATOR_HXX_
#define SPECTATOR_HXX_

#include "src/sim/game_object.hxx"
#include "src/sim/objdl.hxx"

class Ship;

/**
 * Invisible, non-physical GameObject used for spectating.
 *
 * The Spectator is a "GameObject" that tracks another object, but is
 * invisible and non-physical. Its sole purpose is to be used as a GameEnv
 * reference.
 *
 * It is created with an initial Ship as reference; it duplicates this object's
 * position, velocity, and rotation (but not rotational velocity) and tracks
 * information for 5 seconds or until a reference change is requested. At any
 * time, it may be requested to change reference, in which case it immediately
 * jumps to a random appropriate ship. Whenever its referce dies, it has the
 * same effect as when it was first created.
 *
 * It will never go out of bounds.
 */
class Spectator: public GameObject {
  ObjDL ref;

  float timeWithoutReference;
  unsigned long insignia;
  bool isInsigniaRequired;

  float theta;

  bool isAlive;

  public:
  /** Constructs a new Spectator with the given Ship as
   * initial reference.
   * @param s The initial reference. May not be NULL.
   * @param loseImmediate If true, immediately assume the reference is dead.
   */
  Spectator(Ship* s, bool loseImmediate = true);

  /** Constructs a new Spectator in the given GameField, placed at the origin. */
  Spectator(GameField*);

  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual float getRadius() const noth;
  virtual float getRotation() const noth;

  /** Moves to a different alive ship immediately. */
  void nextReference();

  /** Requires later references to have the specified insignia.
   * If no ship has this insignia, behaviour is the same as if this call
   * were never performed.
   */
  void requireInsignia(unsigned long);

  /** Causes the Spectator to return false on all further calls to update(). */
  void kill();

  /** Returns the current Ship* the spectator is following.
   * May return NULL.
   */
  Ship* getReference() const;
};

#endif /* SPECTATOR_HXX_ */
