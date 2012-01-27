/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIModule class
 */

/*
 * aimod.hxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#ifndef AIMOD_HXX_
#define AIMOD_HXX_

#include <libconfig.h++>

#include "src/core/aobject.hxx"

class AIControl;
class GameObject;
class Ship;

/** AIModule is the base class of all modules for the AI controller.
 * Tcl should not be allowed to extend it directly. Instead, an
 * intermediate module, AIM_Tcl, should be created.
 */
class AIModule: public AObject {
  protected:
  /** The AIControl that owns this module */
  AIControl& controller;
  /** The Ship the controller controls */
  Ship*const ship;

  /** Constructor that just sets the controller and ship variables. */
  AIModule(AIControl*);

  public:
  /** Called when the AIModule is in a state which is being activated.
   * It is safe to expose this to Tcl.
   * Default does nothing.
   */
  virtual void activate() {}

  /** Called when the AIModule is in a state which is being deactivated.
   * It is safe to expose this to Tcl.
   * Default does nothing.
   */
  virtual void deactivate() {}

  /** Returns true if this module requires proper activation/deactivation.
   * This means that calls to action() will only work between calls to
   * activate() and deactivate().
   * Default returns false. Modules that return true cannot be used in
   * procedures.
   */
  virtual bool requiresActivateDeactivate() const { return false; }

  /** Called for the primary action of the AIModule.
   * It is safe to expose to Tcl.
   * This is pure-virtual.
   */
  virtual void action() = 0;

  /** Called on requested auto-repeating timer.
   * Argument is the value passed to AIControl::addTimer.
   * It is safe to expose to Tcl.
   * Default does nothing.
   */
  virtual void timer(int) {}

  /** Called on requested IAIC interrupt.
   * Argument is the value passed to AIControl::addInterrupt.
   * It is safe to expose to Tcl.
   * Default does nothing.
   */
  virtual void iaic(int) {}

  /** Used by the learning AI. Requests that the module
   * perform some kind of mutation to its parameters, including
   * its weight.
   * Do not expose to Tcl in any way.
   * Default does nothing.
   */
  virtual void mutate(libconfig::Setting&) {}
};

#endif /* AIMOD_HXX_ */
