/**
 * @file
 * @author Jason Lingle
 * @brief Contains the EnergyCharge weapon
 */

#ifndef ENERGY_CHARGE_HXX_
#define ENERGY_CHARGE_HXX_
#include <vector>
#include <cmath>

#include "src/sim/game_object.hxx"
#include "explode_listener.hxx"

class Ship;

#define EC_SPEED 0.001f ///< Launch speed of EnergyCharge
#define EC_DEGREDATION 0.000125f ///< Rate of energy degradation
#define EC_RADW (STD_CELL_SZ/2) ///< Width of EnergyCharge
#define EC_RADH (EC_RADW/3) ///< Height of EnergyCharge
#define EC_CRRAD (EC_RADW*/*std::sqrt(2.0f)*/1.41421356f) ///< Collision radius of EnergyCharge

/** The EnergyCharge is a burst of energy that becomes progressively weaker as it travels.
 *
 * Its strength is represented by colour, which is magenta at 100% and red near 0%.
 */
class EnergyCharge: public GameObject {
  friend class INO_EnergyCharge;
  friend class ENO_EnergyCharge;
  friend class ExplodeListener<EnergyCharge>;

  private:
  ExplodeListener<EnergyCharge>* explodeListeners;

  //0..1
  const Ship * const parent;
  float intensity;
  const float theta;
  const float tcos, tsin;
  CollisionRectangle collisionRectangle;
  bool exploded;

  unsigned blame;

  //For use by net code
  EnergyCharge(GameField* field, float x, float y, float vx, float vy,
               float _theta, float _inten);

  public:
  /** Constructs a new EnergyCharge with the given parms.
   *
   * @param field The field the charge will live in
   * @param parent The Ship that launched the charge
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param theta Launch direction
   * @param intensity Initial intensity
   */
  EnergyCharge(GameField* field, const Ship* parent, float x, float y, float theta, float intensity);
  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual CollisionResult checkCollision(GameObject*) noth;
  //The default now does what we want
  //virtual vector<CollisionRectangle*>* getCollisionBounds() noth;
  virtual bool collideWith(GameObject*) noth;
  virtual float getRotation() const noth;
  virtual float getRadius() const noth;

  /** Returns the alpha component of a charge's colour for the given intensity. */
  static float getColourA(float) noth;
  /** Returns the red component of a charge's colour for the given intensity. */
  static float getColourR(float) noth;
  /** Returns the green component of a charge's colour for the given intensity. */
  static float getColourG(float) noth;
  /** Returns the blue component of a charge's colour for the given intensity. */
  static float getColourB(float) noth;

  /** Returns the intensity an EnergyCharge with the given starting
   * intensity will have after travelling the given distance.
   */
  static float getIntensityAt(float initIntensity, float dist) noth;

  /** Returns the intensity of the EnergyCharge */
  float getIntensity() const noth { return intensity; }
  /** Causes the EnergyCharge to spontaneously explode due to the given object.
   * Only intended for use by networking code (and internally).
   */
  void explode(GameObject*) noth;
};

#endif /*ENERGY_CHARGE_HXX_*/
