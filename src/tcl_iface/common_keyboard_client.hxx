/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CommonKeyboardClient class
 */

/*
 * common_keyboard_client.hxx
 *
 *  Created on: 25.02.2011
 *      Author: jason
 */

#ifndef COMMON_KEYBOARD_CLIENT_HXX_
#define COMMON_KEYBOARD_CLIENT_HXX_

#include "src/core/aobject.hxx"

class HumanController;
/** The CommonKeyboardClient provides a Tcl-friendly interface
 * to detecting and reacting to common keyboard controls that are
 * described in the config file.
 */
class CommonKeyboardClient: public AObject {
  public:
  CommonKeyboardClient();
  /** The "__ exit" button has been triggered.
   * This is most commonly used to exit the state or
   * open a pause menu.
   */
  virtual void exit() {}

  /** The "__ frameXframe" button has been triggered.
   * This is a debugging option which advances the game state
   * 20 ms four times a second.
   */
  virtual void frameXframe() {}

  /** The "__ fast" button has been triggered.
   * This is a debugging option which makes the game run
   * three times faster than normal.
   */
  virtual void fast() {}

  /** The "__ halt" button has been triggered.
   * This is used to pause the game without opening the pause menu.
   */
  virtual void halt() {}

  /** The "__ slow" button has been triggered.
   * This is a debugging option which makes the game run
   * 1/10th normal speed.
   */
  virtual void slow() {}

  /** The "stats" button has been pressed.
   * This is used to display detailed game information.
   */
  virtual void statsOn() {}

  /** The "stats" button has been released.
   * This is used to hide whatever was shown by statsOn().
   */
  virtual void statsOff() {}

  /** Adds bindings to hc_conf */
  void hc_conf_bind();
};

#endif /* COMMON_KEYBOARD_CLIENT_HXX_ */
