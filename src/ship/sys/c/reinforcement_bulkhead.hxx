/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ReinforcementBulkhead system
 */

#ifndef REINFORCEMENT_BULKHEAD_HXX_
#define REINFORCEMENT_BULKHEAD_HXX_

#include "src/ship/sys/ship_system.hxx"

/** A ReinforcementBulkhead is a localized field that doubles (2.5, actually) the
 * strength of its container.
 *
 * Unlike normal reinforcement, it will
 * not double the mass, although it does consume a fixed amount of
 * energy. It, like ShieldGenerator, responds to shield_deactivate
 * to terminate its protection and energy usage. However, it is
 * not classified as a shield generator. It also does not respond
 * to shield_enumerate, since it does not provide a Shield.
 */
class ReinforcementBulkhead : public ShipSystem {
  private:
  bool active;

  public:
  ReinforcementBulkhead(Ship* s, bool stealth=true);

  virtual unsigned mass() const noth;
  virtual int normalPowerUse() const noth;
  virtual void shield_deactivate() noth;
  virtual float reinforcement_getAmt() noth;
  DEFAULT_CLONE(ReinforcementBulkhead)
};

#endif /*REINFORCEMENT_BULKHEAD_HXX_*/
