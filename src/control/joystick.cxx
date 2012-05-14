/**
 * @file
 * @author Jason Linge
 * @date 2012.05.13
 * @brief Implementation of joystick abstraction
 */

#include <vector>
#include <string>
#include <iostream>

#include <SDL.h>

#include "joystick.hxx"
#include "src/core/lxn.hxx"

using namespace std;

//Scale joystick trackballs so that a sensitivity of 1 is useful.
//How reasonable is this divisor?
//(My joystick has no trackball, so I can't test this...)
#define BALL_MOTION_DIVISOR 1024.0f

namespace joystick {
  struct Joystick {
    SDL_Joystick* handle;
    string name;
    vector<float> axes[JOYSTICK_NUM_AXIS_TYPES];
    //Using char so vector does not become a bitset
    vector<char> buttons[JOYSTICK_NUM_BUTTON_TYPES];
  };
  static vector<Joystick> joysticks;

  void init() {
    unsigned count = SDL_NumJoysticks();
    for (unsigned i = 0; i < count; ++i) {
      Joystick joy;
      //Obtain the name if possible
      const char* name = SDL_JoystickName(i);
      if (!name)
        SL10N(name,"gui","unknown");
      joy.name = name;
      //Try to open it
      joy.handle = SDL_JoystickOpen(i);
      if (!joy.handle) {
        cerr << "Could not open joystick #" << i
             << " (" << name << "): " << SDL_GetError() << endl;
        continue;
      }

      //Allocate each axis and button group
      joy.axes[Axis ].assign(SDL_JoystickNumAxes (joy.handle), 0.0f);
      joy.axes[BallX].assign(SDL_JoystickNumBalls(joy.handle), 0.0f);
      joy.axes[BallY].assign(SDL_JoystickNumBalls(joy.handle), 0.0f);

      joy.buttons[Button  ].assign(SDL_JoystickNumButtons(joy.handle), 0);
      joy.buttons[HatUp   ].assign(SDL_JoystickNumHats   (joy.handle), 0);
      joy.buttons[HatDown ].assign(SDL_JoystickNumHats   (joy.handle), 0);
      joy.buttons[HatLeft ].assign(SDL_JoystickNumHats   (joy.handle), 0);
      joy.buttons[HatRight].assign(SDL_JoystickNumHats   (joy.handle), 0);

      //Joystick fully ready
      joysticks.push_back(joy);
    }
  }

  void close() {
    for (unsigned i = 0; i < joysticks.size(); ++i)
      SDL_JoystickClose(joysticks[i].handle);

    joysticks.clear();
  }

  unsigned count() {
    return joysticks.size();
  }

  const char* name(unsigned ix) {
    return joysticks[ix].name.c_str();
  }

  unsigned axisCount(unsigned ix, AxisType type) {
    return joysticks[ix].axes[type].size();
  }

  unsigned buttonCount(unsigned ix, ButtonType type) {
    return joysticks[ix].buttons[type].size();
  }

  float axis(unsigned ix, AxisType type, unsigned axis) {
    return joysticks[ix].axes[type][axis];
  }

  bool button(unsigned ix, ButtonType type, unsigned button) {
    return joysticks[ix].buttons[type][button];
  }

  void update() {
    SDL_JoystickUpdate();
    for (unsigned ix = 0; ix < joysticks.size(); ++ix) {
      Joystick& joy = joysticks[ix];

      //Handle conventional axes
      for (unsigned i = 0; i < joy.axes[Axis].size(); ++i) {
        Sint16 val = SDL_JoystickGetAxis(joy.handle, i);
        //Make negative range same as positive
        if (val < 0) ++val;
        joy.axes[Axis][i] = val / 32767.0f;
      }

      //Handle balls
      for (unsigned i = 0; i < joy.axes[BallX].size(); ++i) {
        signed dx, dy;
        if (-1 == SDL_JoystickGetBall(joy.handle, i, &dx, &dy))
          continue; //Continue if an error occurs

        joy.axes[BallX][i] = dx / BALL_MOTION_DIVISOR;
        joy.axes[BallY][i] = dy / BALL_MOTION_DIVISOR;
      }

      //Handle normal buttons
      for (unsigned i = 0; i < joy.buttons[Button].size(); ++i) {
        joy.buttons[Button][i] = SDL_JoystickGetButton(joy.handle, i);
      }

      //Handle hats
      for (unsigned i = 0; i < joy.buttons[HatUp].size(); ++i) {
        Uint8 state = SDL_JoystickGetHat(joy.handle, i);
        joy.buttons[HatUp   ][i] = (state & SDL_HAT_UP);
        joy.buttons[HatDown ][i] = (state & SDL_HAT_DOWN);
        joy.buttons[HatLeft ][i] = (state & SDL_HAT_LEFT);
        joy.buttons[HatRight][i] = (state & SDL_HAT_RIGHT);
      }
    }
  }
}
