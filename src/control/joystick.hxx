#ifndef JOYSTICK_HXX_
#define JOYSTICK_HXX_

/**
 * @file
 * @author Jason Linge
 * @date 2012.05.13
 * @brief Abstracts the joystick interface provided by SDL into something
 * easier to use.
 *
 * Balls are abstracted to separate axes, and hats to groups of four buttons.
 */

/** @see joystick.hxx */
namespace joystick {
  /** Defines the various axis types. */
  enum AxisType {
    /**
     * The axis type for a conventional joystick axis.
     */
    Axis=0,
    /**
     * The axis type for the X axis of a joystick ball.
     */
    BallX,
    /**
     * The axis type for the Y axis of a joystick ball.
     */
    BallY
  };
  /**
   * The number of axis types defined.
   */
  #define JOYSTICK_NUM_AXIS_TYPES 3

  /** Defines the various button types. */
  enum ButtonType {
    /**
     * The button type for a conventional button.
     */
    Button=0,
    /**
     * The button type for a hat-up button.
     */
    HatUp,
    HatDown, ///The button type for a hat-down button
    HatLeft, ///The button type for a hat-left button
    HatRight ///The button type for a hat-right button
  };
  ///The number of button types defined
  #define JOYSTICK_NUM_BUTTON_TYPES 5

  /**
   * Initialises the joystick system and opens all joysticks.
   *
   * This should be called at program startup. It does NOT call SDL_Init().
   */
  void init();

  /**
   * Closes the joystick system and closes all joysticks.
   *
   * This should be called at program termination.
   */
  void close();

  /**
   * Returns the number of joysticks open.
   */
  unsigned count();

  /**
   * Returns the name of the joystick at the given index.
   */
  const char* name(unsigned);

  /**
   * Returns the number of axes of the given type supported by the joystick at
   * the given index.
   *
   * This will always return the same value for BallX and BallY.
   */
  unsigned axisCount(unsigned, AxisType);

  /**
   * Returns the number of buttons of the given type supported by the joystick
   * at the given index.
   *
   * This will always return the same value for HatUp, HatDown, HatLeft, and
   * HatRight.
   */
  unsigned buttonCount(unsigned, ButtonType);

  /**
   * Returns the value (-1.0..+1.0) of the axis of the given type and index of
   * the given joystick.
   */
  float axis(unsigned joystick, AxisType, unsigned axis);

  /**
   * Returns the state of the button of the given type and index of the given
   * joystick.
   */
  bool button(unsigned joystick, ButtonType, unsigned button);

  /**
   * Updates internal joystick state.
   *
   * This should be called once per frame.
   */
  void update();
}

#endif /* JOYSTICK_HXX_ */
