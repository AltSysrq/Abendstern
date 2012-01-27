/**
 * @file
 * @author Jason Lingle
 * @brief Contains the abstract Controller class.
 */

#ifndef CONTROLLER_HXX_
#define CONTROLLER_HXX_

#include "src/core/aobject.hxx"
#include "src/opto_flags.hxx"

class Ship;

/** Controller is an abstract class used for defining a human or AI controller of a Ship.
 * It is responsible for all control of the ship; without a controller, no state in the
 * Ship will change, other than automatic responses (such as cutting engines on low power).
 */
class Controller : public AObject {
  public:
  /** The Ship the Controller controls */
  Ship*const ship;

  /** Constructs the Controller with the given Ship */
  Controller(Ship* s) : ship(s) {}
  public:
  /** Performs updates and control of the Ship.
   * This is called by ship before it does anything else in
   * the last virtual frame of each physical frame.
   * @param et Elapsed time, in milliseconds, since the last call
   * to update().
   */
  virtual void update(float et) noth = 0;

  /**
   * Informs the controller about damage in the given direction,
   * relative to the bridge.
   * Default does nothing.
   */
  virtual void damage(float amt, float xoff, float yoff) noth {}

  /**
   * Notifies the controller that another Ship in the field has died.
   * Default does nothing.
   */
  virtual void otherShipDied(Ship*) noth {}

  /**
   * Notifies the controller that it has scored.
   * Default does nothing.
   */
  virtual void notifyScore(signed amt) noth {}

  virtual ~Controller() {};
};

#endif /*CONTROLLER_HXX_*/
