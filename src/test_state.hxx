/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains a simple GameState used to test the game.
 *
 * This file will NOT be included in the final version.
 * As such, it is mostly undocumented.
 */

#ifndef TEST_STATE_HXX_
#define TEST_STATE_HXX_

#include <GL/gl.h>
#include <SDL.h>

#include "core/game_state.hxx"
#include "sim/game_env.hxx"
#include "ship/ship.hxx"
#include "control/human_controller.hxx"

namespace test_state {
  extern const char* gameClass;
  extern int humanShip;
  enum Mode {
    StandardTeam, HomogeneousTeam, HeterogeneousTeam, FreeForAll, LastManStanding, ManVsWorld
  } extern mode;
  enum Background { StarField, Planet, Nebula };
  extern int size;
};

class TestState : public GameState {
  public:
  GameEnv env;
  HumanController* human;

  //These need to be volatile because they may change during
  //the course of the update(float) function.
  volatile Ship* craft_ptr;
  #define craft (*const_cast<Ship*>(craft_ptr))

  int numTestAIDeaths, numStdAIDeaths;

  public:
  TestState(test_state::Background);
  virtual ~TestState();
  virtual GameState* update(float et);
  virtual void draw();
  virtual void mouseButton(SDL_MouseButtonEvent*);
  virtual void motion(SDL_MouseMotionEvent*);
  virtual void keyboard(SDL_KeyboardEvent*);

  void newEnemy();
  void newCraft();
  void newCinema();
};

#endif /*TEST_STATE_HXX_*/
