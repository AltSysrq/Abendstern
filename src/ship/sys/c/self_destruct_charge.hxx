/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SelfDestructCharge system
 */

#ifndef SELF_DESTRUCT_CHARGE_HXX_
#define SELF_DESTRUCT_CHARGE_HXX_

#include "src/ship/sys/ship_system.hxx"

/** A SelfDestructCharge is a system that can be detonated by
 * the Controller at will.
 *
 * It emits a powerful, but short-ranged,
 * Blast with a strength equal to the maxDamage of the container.
 *
 * If damaged/destroyed (without being detonated), there is a 1%
 * chance it will explode anyway.
 */
class SelfDestructCharge : public ShipSystem {
  private:
  bool detonated;
  bool willExplodeWhenDamaged;
  /* If we just exploded when selfDestruct() was called, the Blast
   * would not hit the Ship until /after/ the Ship had been updated
   * once. By doing this, we guarantee that the Blast hits in the
   * location we think it should.
   * Actually, having an update method is flag enough.
   */
  //bool aboutToExplode;

  public:
  SelfDestructCharge(Ship*);
  virtual unsigned mass() const noth;

  virtual void selfDestruct() noth;
  virtual bool damage() noth;
  virtual void destroy() noth;
  virtual int normalPowerUse() const noth { return 0; }

  DEFAULT_CLONE(SelfDestructCharge)

  private:
  void explode() noth;
  static void update(ShipSystem*,float) noth;
};


#endif /*SELF_DESTRUCT_CHARGE_HXX_*/
