/**
 * @file
 * @author Jason Lingle
 * @brief Contains the InitState GameState, which boots the game
 */

#ifndef INIT_STATE_HXX_
#define INIT_STATE_HXX_

#include <GL/gl.h>

#include "game_state.hxx"

class Font;

/**
 * The InitState class is responsible for booting the game.
 * It performs the following operations:
 * <ol>
 *   <li>Calls internal initialisation functions</li>
 *   <li>Loads fonts</li>
 *   <li>Creates a new Tcl interpreter and sources autoexec.tcl</li>
 * </ol>
 */
class InitState: public GameState {
  private:
  GLuint logo;
  GLuint vao, vbo;

  //Don't load anything until we've painted the logo
  bool hasPainted;

  //Steps to load
  float (*const* steps)();
  unsigned numSteps;

  //Keep track of our location in the steps we must do
  unsigned currStep;

  float currStepProgress;

  public:
  /**
   * Constructs a new InitState.
   */
  InitState();
  virtual ~InitState();
  virtual GameState* update(float);
  virtual void draw();
  virtual void keyboard(SDL_KeyboardEvent*);

  private:
  /* Each of these functions performs a short loading action before
   * returning that step's progress.
   * Returns > 1.0 to indicate completion.
   */
  static float miscLoader();
  static float initFontLoader();
  static float starLoader();
  static float systexLoader();
  static float backgroundLoader();
  static float keyboardDelay();
};

#endif /*INIT_STATE_HXX_*/
