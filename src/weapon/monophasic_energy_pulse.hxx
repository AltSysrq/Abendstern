/**
 * @file
 * @author Jason Lingle
 * @brief Contains the MonophasicEnergyPulse weapon
 */

#ifndef MONOPHASIC_ENERGY_PULSE_HXX_
#define MONOPHASIC_ENERGY_PULSE_HXX_

/*
 * monophasic_energy_pulse.hxx
 *
 *  Created on: 12.02.2011
 *      Author: jason
 */

#include "src/sim/game_object.hxx"
#include "explode_listener.hxx"

class Ship;

#define MONO_POWER_MUL 2.0f ///< Damage multiplier for MonophasicEnergyPulse

/** The MonophasicEnergyPulse is a precise, unguided weapon which phases in
 * and out of reality.
 *
 * Its existence follows a sine wave, and collision is
 * only possible during the (physical) frame in which the peak has been
 * reached (specifically, it becomes collideable in the vframe the specific
 * angle that causes a peak is passed, and remains so until the first vframe
 * after the end of the current physical frame).
 *
 * Speed is directly affected by energy level. Waves are fixed with respect
 * to time; because of this, wavelength is also directly affected by energy
 * level.
 *
 * Unlike most weapons, which progressively lose strength over time, the
 * MonophasicEnergyPulse simply ceases to exist at the zero-point of a
 * randomly selected wave.
 */
class MonophasicEnergyPulse: public GameObject {
  friend class INO_MonophasicEnergyPulse;
  friend class ENO_MonophasicEnergyPulse;
  friend class ExplodeListener<MonophasicEnergyPulse>;

  ExplodeListener<MonophasicEnergyPulse>* explodeListeners;
  
  unsigned deathWaveNumber;
  float timeAlive;
  float power;

  /* Set to true every final vframe of the physical frame, and
   * false otherwise. When this is true at the beginning of an
   * update, set includeInCollisionDetection to false.
   */
  bool previousFrameWasFinal;

  /* We don't want to collide with our parent.
   * We never dereference this, so it's safe to have a
   * dangling pointer.
   */
  Ship* parent;

  CollisionRectangle colrect;

  bool exploded;

  unsigned blame;

  //Networking constructor
  MonophasicEnergyPulse(GameField*, float x, float y, float vx, float vy, float pow, unsigned el);

  public:
  /** Constructs a MonophasicEnergyPulse with the given parms.
   *
   * @param field Field in which the pulse lives
   * @param par Ship which fired the pulse
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param theta Direction of firing
   * @param el Energy level
   */
  MonophasicEnergyPulse(GameField* field, Ship* par, float x, float y, float theta, unsigned et);

  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual float getRadius() const noth;

  virtual CollisionResult checkCollision(GameObject* other) noth;
  virtual bool collideWith(GameObject* other) noth;
  //virtual const std::vector<CollisionRectangle*>* getCollisionBounds() noth;

  /** Returns the wavelength at the given energy level */
  static float getWavelength(unsigned el) noth;
  /** Returns the probability of a pulse's survival at the given distance */
  static float getSurvivalProbability(unsigned el, float dist) noth;
  /** Returns the speed at the given energy level */
  static float getSpeed(unsigned el) noth;

  private:
  void explode(GameObject*) noth;
};

#endif /* MONOPHASIC_ENERGY_PULSE_HXX_ */
