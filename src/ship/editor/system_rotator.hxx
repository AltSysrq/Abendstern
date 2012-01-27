/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SystemRotator mode
 */

/*
 * system_rotator.hxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#ifndef SYSTEM_ROTATOR_HXX_
#define SYSTEM_ROTATOR_HXX_

#include <string>

#include "manipulator_mode.hxx"

/** The SystemRotator allows the user to rotate orientable systems
 * off of their defaults. When the user clicks on a cell, the orientation
 * of an orientable system is incremented, then the ship reloaded. If
 * that orientation is not legal, the process is repeated. If no system
 * is orientable, nothing happens.
 *
 * To deal with cells with multiple orientable systems elegantly, the
 * rotator keeps track of the most recently targetted cell and the
 * rotation count. Every time a system is rotated, even unsuccessfully,
 * this count is incremented, modulo 8. If it is greater than 3, the
 * rotator will prefer system 1 over 0, if both are orientable. The
 * count is reset to 0 whenever a new cell is targetted.
 */
class SystemRotator: public ManipulatorMode {
  /* The name of the most recently targetted cell */
  std::string prevTarget;

  /* The rotation count */
  unsigned rotCount;

  public:
  /** Standard mode constructor */
  SystemRotator(Manipulator*,Ship*const&);
  virtual void activate();
  virtual void press(float,float);
};

#endif /* SYSTEM_ROTATOR_HXX_ */
