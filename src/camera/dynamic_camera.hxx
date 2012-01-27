/**
 * @file
 * @author Jason Lingle
 * @brief Provides the DynamicCamera class
 */

#ifndef DYNAMIC_CAMERA_HXX_
#define DYNAMIC_CAMERA_HXX_

#include "camera.hxx"
#include "effects_handler.hxx"
#include "hud.hxx"
#include "src/opto_flags.hxx"

/**
 * If set to true, all camera vibration will cease.
 * This is useful in debugging, so that slow motion or pausing
 * is actually usable.
 */
extern bool debug_dynamicCameraDisableVibration;

/** The DynamicCamera is a more flexible and natural camera than the FixedCamera.
 *
 * It can adjust the zoom from 2 to 0.2, moves more fluidly, and can rotate the
 * camera with the ship (either by velocity or by actual pointing direction).
 * It is also possible to move the focal point up to vheight/2 ahead of the ship.
 *
 * Additionally, it handles effects, such as vibration from impact.
 *
 * To improve performance, the DynamicCamera dynamically adjusts
 * conf.graphics.detail_level in accordance with zoom. Specifically, zooming out
 * will "reduce" (actually increase) the detail level, since the graphics will
 * be less visible anyway.
 */
class DynamicCamera : public Camera {
  friend class TclDynamicCamera;
  private:
  //used for fluid movement
  float currX, currY, currZ, currT;
  float vibration;
  float vx, vy;

  float zoom;
  public:
  /** Defines the rotational behaviour of the DynamicCamera. */
  enum RotateMode {
    /**
     * Axis-alligned mode.
     *
     * The camera will behave like a more fluid FixedCamera.
     */
    None,
    /** The rotation will be the same as the reference object's.
     *
     * If the reference is spinning to quickly, though, this will
     * lag behind.
     */
    Direction,
    /** The rotation will match the direction of the reference object's.
     *
     * Below a certain speed, this behaves like Direction.
     */
    Velocity
  };
  private:
  RotateMode rotateMode;
  float lookAhead;

  //Used mainly by velocity mode to prevent sudden jumps of rotation
  float targetRotation;

  //Back the detail level up
  unsigned baseDetailLevel;

  HUD hud;

  public:
  /**
   * Creates a new DynamicCamera on the given reference and field.
   * @param ref Initial reference, which may be NULL
   * @param field The field the camera will operate on
   */
  DynamicCamera(GameObject* ref, GameField* field);
  virtual ~DynamicCamera();

  protected:
  virtual void doSetup() noth;

  public:
  virtual void update(float) noth;
  virtual void drawOverlays() noth;
  virtual void reset() noth;

  virtual void impact(float) noth;

  /** Returns the current zoom level. */
  float getZoom() const noth;
  /** Sets the zoom level to the desired amount. */
  void setZoom(float) noth;
  /** Returns the current rotation mode. */
  RotateMode getRotateMode() const noth;
  /** Sets the rotation mode to the desired amount. */
  void setRotateMode(RotateMode) noth;
  /** Returns the current "look-ahead" distance.
   *
   * In the Direction and Velocity RotateModes, the reference is shown
   * this amount down on the screen, to allow the player to better see
   * what is ahead of him. In the None RotateMode, this value has no
   * effect.
   */
  float getLookAhead() const noth;
  /** Sets the look-ahead distance.
   *
   * @see getLookAhead()
   */
  void setLookAhead(float) noth;

  /** Returns the current rotation as the player sees it. */
  float getVisualRotation() const noth { return currT; }

  /** Adds all DynamicCamera-provided bindings to hc_conf. */
  void hc_conf_bind();

  private:
  float getRotation() const noth;
};

#endif /*DYNAMIC_CAMERA_HXX_*/
