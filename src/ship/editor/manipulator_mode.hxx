/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ManipulatorMode abstract class
 */

/*
 * manipulator_mode.hxx
 *
 *  Created on: 05.02.2011
 *      Author: jason
 */

#ifndef MANIPULATOR_MODE_HXX_
#define MANIPULATOR_MODE_HXX_

class Manipulator;
class Ship;

/** A ManipulatorMode is an object that allows the Manipulator
 * to perform actual edits.
 *
 * This class is effectively abstract. Although all its functions
 * have do-nothing defaults, the constructor is protected (and
 * even if it weren't, a plain ManipulatorMode does nothing).
 */
class ManipulatorMode {
  protected:
  /** The Manipulator that owns this mode. */
  Manipulator*const manip;

  /** The working pointer from the Manipulator that created us.
   * This may be changed by the Manipulator at any time.
   */
  Ship*const& ship;

  /** If true, the primary button is currently held down.
   */
  bool pressed;

  /** Constructs a new ManipulatorMode with the given
   * Ship const-pointer reference.
   *
   * All subclasses will generally use this same constructor format, which
   * will not be further documented.
   *
   * @param ulator The Manipulator to run under
   * @param s The initial Ship* to use
   */
  ManipulatorMode(Manipulator* ulator, Ship*const& s)
  : manip(ulator), ship(s), pressed(false)
  { }

  public:
  virtual ~ManipulatorMode() {}

  /** Called when the ManipulatorMode is made the current
   * mode. Default does nothing.
   */
  virtual void activate() {}

  /** Called when the ManipulatorMode, until now the current,
   * ceases to be the current. Default does nothing.
   */
  virtual void deactivate() {}

  /** Called when the primary button is pressed at the given
   * screen coordinates. Default sets pressed to true.
   */
  virtual void press(float,float) { pressed=true; }

  /** Called when the primary button is released at the
   * given screen coordinates. Default sets pressed to false.
   */
  virtual void release(float,float) { pressed=false; }

  /** Called when motion occurs, AND immediately after
   * a press with no motion. First pair of arguments
   * are current coordinates, second are difference
   * coordinates.
   * Default does nothing.
   */
  virtual void motion(float,float,float,float) {}

  /** Perform any extra drawing the mode may need to
   * do to indicate its status. Default does nothing.
   * Drawing is performed in field coordinates.
   */
  virtual void draw() {}
};

#endif /* MANIPULATOR_MODE_HXX_ */
