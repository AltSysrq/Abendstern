/**
 * @file
 * @author Jason Lingle
 * @brief Contains the EffectsHandler class
 */

#ifndef EFFECTS_HANDLER_HXX_
#define EFFECTS_HANDLER_HXX_

class Explosion;
class Ship;

#include "src/opto_flags.hxx"
#include "src/core/aobject.hxx"

/** The EffectsHandler is an interface attached to ships that allows
 * for flexible handling of visual and audial effects.
 */
class EffectsHandler: public AObject {
  public:
  /** Called when a ship is impacted by something.
   * The value input is equal to force/mass.
   *
   * Default does nothing.
   *
   * @param amt Acceleration provided by impact
   */
  virtual void impact(float amt) noth {}

  /** Called when an explosion is generated (not any Explosion, but
   * one really intended as such).
   *
   * @param ex Explosion triggering the effect; will be deleted shortly
   * after this call.
   */
  virtual void explode(Explosion* ex) noth {}
};

/** Nonextended EffectsHandler which does nothing. */
extern EffectsHandler nullEffectsHandler;

#endif /*EFFECTS_HANDLER_HXX_*/
