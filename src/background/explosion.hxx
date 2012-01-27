/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the Explosion class.
 */

#ifndef EXPLOSION_HXX_
#define EXPLOSION_HXX_

#include <GL/gl.h>
#include <new>

#include "src/sim/game_object.hxx"
#include "src/opto_flags.hxx"

/* These were chosen to discard a certain amount
 * of precision.
 */
#define DENSITY_IMULT (1 << 20)
#define SMEARFAC_IMULT (1 << 14)
#define SIZE_IMULT (1 << 20)

/** Internal use only.
 * This is returned by Explosion::operator new().
 */
extern void* staticExplosion;

/** An Explosion is simply a spread of particles, and is strictly
 * a visual effect.
 *
 * An explosion is created according to its colour,
 * size after 1 second, XY velocity, lifetime, and density.
 * Density is the relation to the number of pixels on the screen
 * to the number of particles in the explosion (eg, a density
 * of 0.01 on a 1024x768 screen would generate 7643 particles).
 * Since all particles expand from each other at fixed rates,
 * the actual shape of an explosion never changes. This means we
 * can actually store the explosion as it will be at 1 second into
 * a display list, and just scale it from there.
 *
 * It turns out that representing these as GameObjects puts too much
 * load on the system. Therefore, this class is now only a proxy to
 * GameField's internal ExplosionPool interface.
 *
 * Since existing code never creates more than one simultaneous
 * heap Explosion at a time, and repeated allocations/deletions turned
 * out to be quite a load, operator new() and operator delete() are
 * both overloaded to essentially do nothing. It is therefore only
 * possible to have one instance of this class in the heap at a time.
 */
class Explosion : public GameObject {
  friend class ExplosionPool;
  public:
  /** Defines the type of an Explosion.
   * Multiple types of explosions are supported, each represented
   * by one of these values.
   */
  enum ExplosionType {
    /** Simple is a basic circular gradient.
     * It is not sensitive to density or elapsed time.
     */
    Simple=0,
    /** Sparkle is a variant on the spark explosion that
     * changes several times a second. It should not be
     * used for large explosions. It is sensitive to
     * density and elapsed time.
     */
    Sparkle,
    /** Spark is the original type of explosion, a bunch
     * of points in the main circle. It is sensitive to
     * density, but not elapsed time.
     * This version should only be used for small to
     * medium-sized explosions.
     */
    Spark,
    /** Modification of Spark for large explosions. */
    BigSpark,
    /** Incursion is a dramatic explosion similar to the
     * effect some Sci-Fi movies use for power core
     * implosions and the like. It is sensitive to
     * elapsed time, but not density.
     */
    Incursion,
    /** Flame is a flame-like explosion. It is sensitive
     * to elapsed time, but not density.
     */
    Flame,
    /** Used to simplify programming. The Invisible explosion
     * is discarded immediately after informing the effects
     * handler about it.
     */
    Invisible
  };

  private:
  GLfloat colour[3];
  float lifetime;
  float sizeAt1Sec;

  //Save some parms for multiExplosion()
  float density;

  float smearFactor, smearAngle;

  ExplosionType type;

  public:
  /** The effects handler uses this value instead of the true density.
   * This is useful for Engines so they don't glare the screen too much.
   */
  float effectsDensity;

  /** A "hungry" explosion will cause all other explosions of the same type,
   * within a certain distance, to be silently discarded (although the
   * EffectsHandler will be notified). Defaults to false.
   */
  bool hungry;

  public:
  /** Creates a new Explosion with the specified parms.
   *
   * @param field The field to add the Explosion to
   * @param type The type of the Explosion
   * @param red The initial red component of the colour
   * @param green The initial green component of the colour
   * @param blue The initial blue component of the colour
   * @param sizeAt1Sec The size of the Explosion at one second
   * @param density The density of the effect (meaning varies by type)
   * @param lifetime The number of milliseconds the Explosion lives
   * @param x The initial X coordinate
   * @param y The initial Y coordinate
   * @param vx The X velocity
   * @param vy The Y velocity
   * @param smearFactor Amount to smear the effect (support limited)
   * @param smearAngle Angle to smear the effect
   */
  Explosion(GameField* field, ExplosionType type,
            GLfloat red, GLfloat green, GLfloat blue, float sizeAt1Sec, float density,
            float lifetime, float x, float y, float vx=0, float vy=0,
            //"Smearing" is used by engines so that the trail doesn't become chunky
            float smearFactor=0, float smearAngle=0);
  /** Creates a new Explosion with the specified parms at a GameObject.
   *
   * @param field The field to add the Explosion to
   * @param type The type of the Explosion
   * @param red The initial red component of the colour
   * @param green The initial green component of the colour
   * @param blue The initial blue component of the colour
   * @param sizeAt1Sec The size of the Explosion after one second
   * @param density The density of the effect (meaning varies by type)
   * @param lifetime The number of milliseconds the Explosion lives
   * @param reference An object whose X, Y, VX, and VY values are used
   *   for the same in this Explosion
   */
  Explosion(GameField* field, ExplosionType type,
            GLfloat red, GLfloat green, GLfloat blue, float sizeAt1Sec, float density,
            float lifetime, GameObject* reference);
  /** Copy constructor. */
  Explosion(const Explosion&);
  virtual ~Explosion();

  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual CollisionResult checkCollision(GameObject* other) noth;
  virtual float getRadius() const noth;
  /** Adds several similar, but slightly different, explosions to the given field.
   * The variations in colour will make explosions more realistic.
   * This explosion itseld should not be placed into the field.
   *
   * @param count The number of Explosions to create
   */
  void multiExplosion(int count) noth;

  /** Returns the red colour component */
  float getColourR() const noth { return colour[0]; }
  /** Returns the green colour component */
  float getColourG() const noth { return colour[1]; }
  /** Returns the blue colour component */
  float getColourB() const noth { return colour[2]; }
  /** Returns the size-after-one-second */
  float getSize() const noth { return sizeAt1Sec; }
  /** Returns the effect density */
  float getDensity() const noth { return density; }
  /** Returns the maximum lifetime of the Explosion */
  float getLifetime() const noth { return lifetime; }

  /* We can take advantage of the fact that now
   * there will never be more than one new-
   * allocated Explosion in existence, and
   * just use static memory allocation instead
   * of dynamic.
   */
  /** Pretends to allocate a new Explosion.
   * In reality, it just returns staticExplosion.
   * @param sz ignored
   */
  void* operator new(size_t sz) throw (std::bad_alloc) {
    return staticExplosion;
  }
  /** Pretends to deallocate the given Explosion.
   * In reality, does nothing.
   * @param px ignored
   */
  void operator delete(void* px) throw() {}
};

#endif /*EXPLOSION_HXX_*/
