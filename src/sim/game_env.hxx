/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GameEnv class
 */

#ifndef GAME_ENV_HXX_
#define GAME_ENV_HXX_

#include "game_field.hxx"
#include "src/opto_flags.hxx"
#include "src/core/aobject.hxx"

class Background;
class Camera;
class GameObject;

/** The GameEnv manages the starfield, gamefield, and camera.
 */
class GameEnv: public AObject {
  public:
  /** The field that all GameObjects will be placed into. */
  GameField field;
  /** The background.
   *
   * This MUST be initialised by client code; its initial value is undefined.
   */
  Background* stars;
  /** The camera to use. */
  Camera* cam;

  /**
   * Constructs a new GameEnv with the given parameters.
   * @param cam The Camera to use
   * @param width The width of the field
   * @param height The height of the field
   */
  GameEnv(Camera* cam, float width, float height);
  /**
   * Constructs a new GameEnv with a DynamicCamera.
   * @param width The width of the field
   * @param height The height of the field
   */
  GameEnv(float width, float height);
  /** Returns the current reference object of the camera
   * and the background.
   */
  GameObject* getReference() noth;
  /** Alters the reference object of the camera and the
   * background, reseting them if requested.
   */
  void setReference(GameObject*, bool reset);

  /** Updates everything the GameEnv manages.
   *
   * @param et Elapsed time, in milliseconds, since the previous call to update()
   */
  void update(float et);
  /** Draws everything the GameEnv manages. */
  void draw();

  /** Allows Tcl to access the field properly. */
  GameField* getField() { return &field; }

  virtual ~GameEnv();
};

#endif /*GAME_ENV_HXX_*/
