/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CloakingDevice system
 */

/*
 * cloaking_device.hxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#ifndef CLOAKING_DEVICE_HXX_
#define CLOAKING_DEVICE_HXX_

#include "src/ship/sys/ship_system.hxx"

/** A CloakingDevice greatly improves a ship's stealth ability, including enabling
 * it to become 100% transparent. Actual logic for cloaking itself is handled within
 * Ship. This class merely presents the existence of the device and draws no power
 * on its own.
 */
class CloakingDevice: public ShipSystem {
  public:
  /** Constructs a CloakingDevice for the given Ship. */
  CloakingDevice(Ship*);
  virtual unsigned mass() const noth;
  virtual signed normalPowerUse() const noth { return 0; }
  DEFAULT_CLONE(CloakingDevice);
};

#endif /* CLOAKING_DEVICE_HXX_ */
