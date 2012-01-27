/**
 * @file
 * @author Jason Lingle
 * @brief Contains the old FixedCamera class
 */

#ifndef FIXED_CAMERA_HXX_
#define FIXED_CAMERA_HXX_

#include "../sim/game_object.hxx"
#include "camera.hxx"

/** The fixed camera is ridgedly attached to its reference, in the
 * centre of the screen.
 * It does not rotate with it.
 */
class FixedCamera : public Camera {
  public:
  /**
   * Constructs a new FixedCamera on the given reference object.
   * @param ref The reference, which may be NULL.
   */
  FixedCamera(GameObject* ref) : Camera(ref) {}

  protected:
  virtual void doSetup() noth;
};

#endif /*FIXED_CAMERA_HXX_*/
