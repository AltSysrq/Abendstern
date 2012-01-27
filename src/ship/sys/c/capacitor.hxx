/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Capacitor system
 */

#ifndef CAPACITOR_HXX_
#define CAPACITOR_HXX_

#include "src/ship/sys/ship_system.hxx"

#define CAPACITOR_MAX 100

/** A Capacitor builds up spare energy, and discharges
 * it when needed. The Ship, not the Capacitor, stores
 * the actual amount of energy; the Capacitor itself
 * is merely to take a system slot and for the graphic.
 */
class Capacitor : public ShipSystem {
  private:
  unsigned capacity;

  public:
  Capacitor(Ship* parent, unsigned cap=30);
  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth { return 0; }
  unsigned getCapacity() noth;
  virtual ShipSystem* clone() noth;
  //Returns whether the value was accepted
  bool setCapacity(unsigned cap) noth;
  virtual void write(std::ostream&) noth;

  virtual void audio_register() const noth;
};

#endif /*CAPACITOR_HXX_*/
