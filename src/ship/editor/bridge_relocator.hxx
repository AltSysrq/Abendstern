/**
 * @file
 * @author Jason Lingle
 * @brief Contains the BridgeRelocator mode
 */

/*
 * bridge_relocator.hxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#ifndef BRIDGE_RELOCATOR_HXX_
#define BRIDGE_RELOCATOR_HXX_

#include "manipulator_mode.hxx"

/** The BridgeRelocator allows the user to relocate the bridge by
 * clicking on the new cell. This performs any checks and adjustments
 * to ensure that this is both legal and unsurprising:
 * + The destination must have no systems
 * + The destination must have a rotation which is an integer multiple
 *   of its side theta (90 for square/circle and 120 for equilateral)
 * + The destination must not be a right-triangle
 * Additionally, if the destination has non-zero rotation, its neighbour
 * slots must be rotated so that it will have zero rotation.
 *
 * After successful relocation, it sets the mode to "none".
 */
class BridgeRelocator: public ManipulatorMode {
  public:
  /** Standard mode constructor */
  BridgeRelocator(Manipulator*,Ship*const&);
  virtual void activate();
  virtual void press(float,float);
};

#endif /* BRIDGE_RELOCATOR_HXX_ */
