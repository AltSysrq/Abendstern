/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GameState class
 */

#ifndef GAME_STATE_HXX_
#define GAME_STATE_HXX_
#include <SDL.h>

#include "aobject.hxx"

/**
 * Encapsulates the updating, drawing, and input processing for a state of the game.
 */
class GameState: public virtual AObject {
  public:
  virtual ~GameState();

  /** Updates the state. This function should normally return NULL; if it
   * returns non-NULL, that usually indicates a request to terminate the
   * state. Generally, returning a GameState* will switch to that state,
   * although behaviour is controller-dependant. The root system will
   * assume that any non-NULL pointer is a GameState*, and will switch to
   * it if it is returned.
   *
   * This function is called before the draw() function each frame. However,
   * with a root-level switch, this may not be called the first time.
   *
   * @param elapsedTimeMillis Time since last call to update() in milliseconds
   */
  virtual GameState* update(float elapsedTimeMillis)=0;

  /** Draws the current frame. This should assume most of the GL context
   * has been set up (as it should have by configureGL()).
   */
  virtual void draw()=0;

  /** Configures the OpenGL context. This is always called at least once
   * before update(float) or draw(), and is called whenever the window
   * is resized. GL has already be configured for the size of the window.
   *
   * The default configures an orthographic 2D projection from (0,0) to (1,1).
   */
  virtual void configureGL();

  /**
   * Processes a single keyboard event from SDL.
   * Default does nothing.
   */
  virtual void keyboard(SDL_KeyboardEvent* e) {}
  /**
   * Processes a single mouse button event from SDL.
   * Default does nothing.
   */
  virtual void mouseButton(SDL_MouseButtonEvent* e) {}
  /**
   * Processes a single mouse motion event from SDL.
   * Default does nothing.
   */
  virtual void motion(SDL_MouseMotionEvent* e) {}
};

#endif /*GAME_STATE_HXX_*/
