/**
 * @file
 * @author Jason Lingle
 * @brief Defines the abstract Camera class
 */

#ifndef CAMERA_HXX_
#define CAMERA_HXX_

#include "src/sim/game_object.hxx"
#include "src/opto_flags.hxx"
#include "effects_handler.hxx"

/** The Camera controls OpenGL transforms with respect to the
 * player's view.
 */
class Camera: public EffectsHandler {
  friend class TclCamera;
  protected:
  /** The GameObject to follow */
  GameObject* reference;
  /** Constructs the Camera with the specified initial reference.
   *
   * @param ref Initial reference; may be NULL
   */
  Camera(GameObject* ref) : reference(ref) {}
  /** Called by setup(); it does the actual work for the Camera.
   *
   * When this is called, the identity will already have been loaded, and matrix
   * mode switched to projection.
   */
  virtual void doSetup() noth = 0;

  public:
  /** Makes any necessary updates to the camera.
   *
   * The default does nothing.
   *
   * @param time Time in milliseconds since the last call to update()
   */
  virtual void update(float time) noth {}
  /** Draw any overlays that may exist.
   *
   * Default does nothing.
   */
  virtual void drawOverlays() noth {}
  /** Resets the camera to the new reference.
   *
   * In a fluid camera, this would immediately
   * move the reference point, instead of slowly
   * moving over.
   *
   * Default does nothing.
   */
  virtual void reset() noth {}
  /** Sets the view matrix up.
   *
   * @param standard If true, the top
   * of the view stack is set to the standard orthographic
   * view before calling doSetup(). Otherwise, doSetup()
   * is called directly.
   */
  void setup(bool standard=true) noth;

  /** Returns the current reference object.
   *
   * @return The reference, which may be NULL.
   */
  GameObject* getReference() const noth;
  /** Alters the Camera's reference.
   *
   * @param ref The new reference, which may be NULL
   * @param reset If true, reset() will be called automatically
   */
  void setReference(GameObject* ref, bool reset=true) noth;
};

#endif /*CAMERA_HXX_*/
