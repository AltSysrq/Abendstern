/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Blast object
 */

#ifndef BLAST_HXX_
#define BLAST_HXX_

#include <vector>

#include "game_object.hxx"
#include "game_field.hxx"
#include "collision.hxx"
#include "src/opto_flags.hxx"

/** A Blast is an invisible GameObject whose lifespan is only one frame.
 *
 * It is created at a specific point, and with a given strength and
 * falloff radius. It's collision bounds are actually a square with
 * side length=radius*2. Anything that collides with it receives damage
 * according to strength*((1-falloff/distance)**2). The other objects
 * are expected to take care of everything; this class merely returns
 * its properties and return MaybeCollision for checkCollision(GameObject*).
 */
class Blast: public GameObject {
  private:
  const float falloff, strength, size;
  CollisionRectangle rect;
  bool isAlive;
  bool direct;
  //When a ship is split by a Blast,
  //this is set to false so that
  //the pieces are blown apart
  //without causing unfair damage
  bool causeDamage;

  /** We need to recreate a decorative copy to collide with decorative
   * objects. The reason for creating a new blast is the same that
   * we have the two-step decoration process (see game_object.hxx).
   */
  Blast(const Blast*);

  public:
  /**
   * Integer ID (specific to the shell) that determines the blame for
   * damage caused by this blast.
   */
  const unsigned blame;

  /** Constructs a Blast with the given parms.
   *
   * @param field The GameField the Blast will live in
   * @param x X coordinate of the centre
   * @param y Y coordinate of the centre
   * @param falloff Distance over which strength decays from 100% to 0
   * @param strength Strength of the Blast
   * @param direct True if Blast was caused by direct contact between objects
   * @param size Inner radius before falloff is applied
   * @param addDecorativeCopy If true, a decorative clone of this Blast will be
   *   automatically added to field
   * @param decorative Whether the Blast is decorative. It does not make sense to
   *   have both decorative and addDecorativeCopy true.
   * @param causesDamage Whether the Blast causes damage or just pushes things
   */
  Blast(GameField* field, unsigned blame, float x, float y, float falloff, float strength,
        bool direct=false, float size=0, bool addDecorativeCopy=true,
        bool decorative=false, bool causesDamage=true);
  /** Creates a possibly non-damaging clone of a Blast.
   *
   * @param that Blast to copy
   * @param damaging Whether the Blast will cause damage
   */
  Blast(const Blast* that, bool damaging);
  virtual bool update(float et) noth;
  virtual void draw() noth;
  //We don't need to override this, as the default works just fine
  //virtual vector<CollisionRectangle*>* getCollisionBounds() noth;
  virtual CollisionResult checkCollision(GameObject*) noth;
  virtual float getRadius() const noth;

  /** Returns the falloff */
  float getFalloff() const noth;
  /** Returns the strength at distance 0 */
  float getStrength() const noth;
  /** Returns the strength at the given distance */
  float getStrength(float distance) const noth;
  /** Returns the strength at other's centre */
  float getStrength(const GameObject* other) const noth;
  /** Returns size */
  float getSize() const noth { return size; }
  /** Returns direct */
  bool isDirect() const noth;
  /** Returns whether the Blast causes damage */
  bool causesDamage() const noth;
};

#endif /*BLAST_HXX_*/
