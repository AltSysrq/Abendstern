/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines the ExplosionPool interface, which manages Explosions.
 *
 * It does so much more efficiently than the GameField if they were treated
 * normally.
 *
 * This is really only useful for GameField.
 *
 * Original rationale (still relevant, but different statistics):
 * The virtual function calls associated with GameObjects is too much for
 * the system to handle if Explosions are treated normally. A 200 fps rate
 * is simulated, so each engine will create 200 Explosions per frame. With
 * seven engines and a half-second (100-frame) lifetime, that results in
 * 700 explosions per frame, or 140000 virtual calls per second.
 * Therefore, we pool Explosion resources externally.
 */

#ifndef EXPLOSION_POOL_HXX_
#define EXPLOSION_POOL_HXX_

#include <GL/gl.h>
#include <vector>
#include <deque>
#include <map>
#include <iostream>

#include "src/opto_flags.hxx"
#include "explosion.hxx"

#define EXPLOSIONS_PER_SEGMENT 1024

//To further speed access, the data is striped as follows:
//  x vx y vy size sizeExpansion
#define EX_X_OFF 0
#define EX_VX_OFF 1
#define EX_Y_OFF 2
#define EX_VY_OFF 3
#define EX_VLEN (EX_VY_OFF+1)

//We also stripe elapsedTime and lifetime
#define EX_ET_OFF 0
#define EX_LT_OFF 1
#define EX_TLEN 2

#define EX_CACHE_SZ 64

class ExplosionPool;

/**
 * This struct is used internally by ExplosionPool.
 * \cond INTERNAL
 */
struct ExplosionPoolSegment {
  float coordStripe[EX_VLEN * EXPLOSIONS_PER_SEGMENT];
  float timeStripe[EX_TLEN * EXPLOSIONS_PER_SEGMENT];
  GLfloat colours[EXPLOSIONS_PER_SEGMENT][4];
  Explosion::ExplosionType types[EXPLOSIONS_PER_SEGMENT];
  float sizes[EXPLOSIONS_PER_SEGMENT];
  float densities[EXPLOSIONS_PER_SEGMENT];

  //Bitset of usage in the segment
  unsigned int freeCount;
  unsigned int usage[EXPLOSIONS_PER_SEGMENT/sizeof(int)/8];

  ExplosionPoolSegment();
  ~ExplosionPoolSegment();
  void update(float) noth OPTIMIZE_ANYWAY;
  void draw() noth OPTIMIZE_ANYWAY;
};

/**
 * Used internally by ExplosionPool.
 */
struct HungryExplosion {
  Explosion::ExplosionType type;
  float minX, maxX, minY, maxY;
  unsigned framesLeft;
};

/** \endcond */

/**
 * The ExplosionPool class does the work of managing Explosions.
 * @see Long description of src/background/explosion_pool.hxx
 */
class ExplosionPool {
  friend class ExplosionPoolSegment;

  std::vector<ExplosionPoolSegment*> segments;

  //We GC when this hits zero
  //We don't care what it's initialized to, because
  //anything's OK
  unsigned short segmentGCCounter;

  /* A collection of HungryExplosions that are still
   * valid.
   */
  std::deque<HungryExplosion> hungryExplosions;

  public:
  /** Creates a new empty ExplosionPool */
  ExplosionPool();
  ~ExplosionPool();

  void update(float) noth OPTIMIZE_ANYWAY;
  void draw() noth OPTIMIZE_ANYWAY;
  /** Internalises, then deletes, the specified Explosion.
   *
   * @param ex Explosion to work with; after this call, it is "deleted"
   */
  void add(Explosion* ex) noth;

  private:
  #ifdef PROFILE
  #define inline
  #endif
  inline void findFreeSegment(ExplosionPoolSegment*&, int&, int&, int&, std::vector<ExplosionPoolSegment*>& segments) noth;
  #ifdef PROFILE
  #undef inline
  #endif
};

#endif /*EXPLOSION_POOL_HXX_*/
