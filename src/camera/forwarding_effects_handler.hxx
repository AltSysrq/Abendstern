/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ForwardingEffectsHandler class
 */

#ifndef FORWARDING_EFFECTS_HANDLER_HXX_
#define FORWARDING_EFFECTS_HANDLER_HXX_

#include "src/sim/game_field.hxx"
#include "effects_handler.hxx"
#include "src/ship/ship.hxx"

/** The forwarding effects handler simply passes events from a Ship to
 * its containing GameField.
 */
class ForwardingEffectsHandler : public EffectsHandler {
  private:
  Ship *const ship;
  public:
  /** Constructs a new ForwardingEffectsHandler for the given Ship.
   *
   * It is not automatically attached to the ship.
   * @param s The ship to operate on.
   */
  ForwardingEffectsHandler(Ship* s);
  virtual void impact(float) noth;
  virtual void explode(Explosion*) noth;
};

#endif /*FORWARDING_EFFECTS_HANDLER_HXX_*/
