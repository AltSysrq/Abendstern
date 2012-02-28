/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MagnetoBomb weapon
 */

#ifndef MAGNETO_BOMB_HXX_
#define MAGNETO_BOMB_HXX_

#include <vector>

#include "src/sim/game_object.hxx"

class Ship;

/// The base speed of a MagnetoBomb
#define MB_SPEED 0.0007f
/** The MagnetoBomb is a magnetically-charged package of antimatter.
 * It is magnetically attracted to every ship according to<br>
 *   ATTRACTION_CONST*ship_mass/dist/dist<br>
 * It's own mass does not need to be factored in, since momentum cancells
 * extra attraction out.
 * Each bomb has a halflife proportional to the square root of its mass,
 * and a three second arming delay (so that it can get out of range of
 * the mothership).
 */
class MagnetoBomb: public GameObject {
  friend class INO_MagnetoBomb;
  friend class ENO_MagnetoBomb;
  private:
  float power;
  float coreRadius;
  /* Each second, there is this probability that the bomb spontaneously
   * explodes. This is actually predetermined now.
   */
  float halflife;
  /* The time until arming. If it is not armed, it collides with nothing.
   */
  float armTime;
  /* Don't arm while we are still within our parent's shields. */
  bool inParentsShields, hitParentsShields;

  /* The bomb is displayed as ball of glares. Each of these represents the
   * distance, from the centre, of one of the spikes.
   */
  float rotation;

  CollisionRectangle collisionRects[8];

  Ship* parent;

  float r, g, b;

  bool exploded;

  /* In order to improve performance, adjust vx and vy by
   * ax and ay every virtual frame, and update ax and ay only
   * once every five physical frames.
   * These are not used by SemiguidedBomb.
   */
  float ax, ay;
  int physFrameCount;

  protected:
  /**
   * Tracks the number of milliseconds the bomb has lived.
   *
   * If we engage full guidance immediately, we take away any
   * advantage to aiming, as we immediately start heading directly
   * for the target.
   *
   * Instead, gradually activate guidance over 1.5s
   */
  float timeAlive;

  /**
   * The blame value to pass on to any Blast.
   */
  unsigned blame;

  public:
  /** Constructs a new MagnetoBomb.
   *
   * @param field The field the bomb will live in
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param vx Initial X velocity
   * @param vy Initial Y velocity
   * @param power Energy level
   * @param parent Ship that fired the weapon
   * @param subMult Subclass only --- multiplies the damage output
   * @param r Subclass only --- overrides red component of colour
   * @param g Subclass only --- overrides green component of colour
   * @param b Subclass only --- overrides blue component of colour
   */
  MagnetoBomb(GameField* field, float x, float y, float vx, float vy,
              float power, Ship* parent, float subMult=1,
              float r=1, float g=1, float b=1);
  virtual bool update(float)  noth HOT;
  virtual void draw() noth;
  virtual CollisionResult checkCollision(GameObject*) noth;
  //Default now does what we want
  //virtual std::vector<CollisionRectangle*>* getCollisionBounds() noth;
  virtual bool collideWith(GameObject*) noth;
  virtual float getRotation() const noth;
  virtual float getRadius() const noth;
  virtual bool isCollideable() const noth;

  /** Returns the power of the bomb */
  float getPower() const noth { return power; }

  /** Called by a launcher in overpower mode. When called,
   * the bomb will explode next frame.
   */
  void simulateFailure() noth;

  protected:
  /** Performs updating common to MagnetoBomb and SemiguidedBomb */
  bool coreUpdate(float) noth;

  private:
  void explode() noth;
};

#endif /*MAGNETO_BOMB_HXX_*/
