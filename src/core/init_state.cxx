/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/core/init_state.hxx
 */

#include <iostream>
#include <typeinfo>
#include <ctime>
#include <new>
#include <algorithm>

#include <GL/gl.h>
#include <SDL.h>

#include <tcl.h>

#include "init_state.hxx"
#include "src/abendstern.hxx"
#include "src/globals.hxx"
#include "src/graphics/imgload.hxx"
#include "src/test_state.hxx"
#include "src/fasttrig.hxx"
#include "src/background/background_object.hxx"
#include "src/background/star_field.hxx"
#include "src/background/explosion_pool.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/mat.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/gl32emu.hxx"
#include "src/graphics/font.hxx"
#include "src/graphics/asgi.hxx"
#include "src/audio/audio.hxx"
#include "src/secondary/namegen.hxx"
#include "src/tcl_iface/bridge.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

#if defined(AB_OPENGL_14)
#define LOGO "images/a14.png"
#elif defined(AB_OPENGL_21)
#define LOGO "images/a21.png"
#else
#define LOGO "images/a.png"
#endif

#define PRELIM_LOGO "images/a.png"

//For LSD-mode Easter Egg
static bool hasPressedL=false, hasPressedS=false, hasPressedD=false;

InitState::InitState() {
  currStep = 0;
  currStepProgress = 0;

  static float (*const stepsToLoad[])() = {
    &InitState::miscLoader,
    &InitState::initFontLoader,
    &InitState::systexLoader,
    &InitState::starLoader,
    &InitState::backgroundLoader,
    &InitState::keyboardDelay,
  };
  steps = stepsToLoad;
  numSteps = lenof(stepsToLoad);

  hasPainted=headless;
  if (!headless) {
    if (!preliminaryRunMode)
      SDL_ShowCursor(SDL_DISABLE);
    glGenTextures(1, &logo);
    vao = newVAO();
    glBindVertexArray(vao);
    vbo = newVBO();
    const char* error=loadImage(preliminaryRunMode? PRELIM_LOGO : LOGO, logo);
    if (error) {
      cerr << error << endl;
      exit(EXIT_MALFORMED_DATA);
    }
    //We can't set the buffer up yet, because vheight
    //has not yet been calculated.
  }
}

InitState::~InitState() {
  if (!headless) {
    glDeleteTextures(1, &logo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }
}

GameState* InitState::update(float) {
  if (!hasPainted && !headless) return NULL;

  if (currStep < numSteps) {
    currStepProgress = steps[currStep]();
    if (currStepProgress > 1.0f) {
      currStepProgress = 0;
      ++currStep;
    }
  } else {
    if (hasPressedL && hasPressedS && hasPressedD) {
      enableLSDMode();
      shader::textureReplace.unload();
      shader::basic.unload();
    }

    //Start the root interpreter
    Tcl_Interp* root=newInterpreter(false);
    if (TCL_ERROR == Tcl_EvalFile(root, "tcl/autoexec.tcl")) {
      cerr << "FATAL: Autoexec did not run successfully: " << Tcl_GetStringResult(root) << endl;
      Tcl_Obj* options=Tcl_GetReturnOptions(root, TCL_ERROR);
      Tcl_Obj* key=Tcl_NewStringObj("-errorinfo", -1);
      Tcl_Obj* stackTrace;
      Tcl_IncrRefCount(key);
      Tcl_DictObjGet(NULL, options, key, &stackTrace);
      Tcl_DecrRefCount(key);
      cerr << "Stack trace:\n" << Tcl_GetStringFromObj(stackTrace, NULL) << endl;
      exit(EXIT_SCRIPTING_BUG);
    }
    if (this == state) {
      cerr << "FATAL: Autoexec did not change GameState" << endl;
      exit(EXIT_SCRIPTING_BUG);
    }
    if (!headless) {
      SDL_EnableUNICODE(1);
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
      //Tcl is taking over in a somewhat rude way, so
      //handle as gracefully as possible
      state->configureGL();
    }
  }

  return NULL;
}

void InitState::draw() {
  hasPainted=true;

  const shader::textureReplaceV quad[4] = {
      { {{0.5f-vheight/2,0      ,0,1}}, {{0,1}} },
      { {{0.5f+vheight/2,0      ,0,1}}, {{1,1}} },
      { {{0.5f-vheight/2,vheight,0,1}}, {{0,0}} },
      { {{0.5f+vheight/2,vheight,0,1}}, {{1,0}} },
  };

  shader::textureReplaceU uni;
  uni.colourMap=0;

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
  shader::textureReplace->setupVBO();
  glBindTexture(GL_TEXTURE_2D, logo);
  shader::textureReplace->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  static const float progressH = 0.02f;
  static const float progressW = 0.3f;
  float currProgress = currStep/(float)numSteps + currStepProgress/numSteps;
  float baseX = 0.5f - progressW/2;
  #if defined(AB_OPENGL_14)
  asgi::colour(1.0f, 0.4f, 0.2f, 0.75f);
  #elif defined(AB_OPENGL_21)
  asgi::colour(0.2f, 1.0f, 0.4f, 0.75f);
  #else
  asgi::colour(0.2f, 0.3f, 0.9f, 0.75f);
  #endif
  asgi::begin(asgi::Quads);
  asgi::vertex(baseX, 4*progressH);
  asgi::vertex(baseX, 5*progressH);
  asgi::vertex(baseX + currProgress*progressW, 5*progressH);
  asgi::vertex(baseX + currProgress*progressW, 4*progressH);
  asgi::end();
  asgi::begin(asgi::Quads);
  asgi::vertex(baseX, 2.75f*progressH);
  asgi::vertex(baseX, 3.75f*progressH);
  asgi::vertex(baseX + currStepProgress*progressW, 3.75*progressH);
  asgi::vertex(baseX + currStepProgress*progressW, 2.75*progressH);
  asgi::end();
}

void InitState::keyboard(SDL_KeyboardEvent* e) {
  if (e->keysym.sym == SDLK_l)
    hasPressedL = true;
  else if (e->keysym.sym == SDLK_s)
    hasPressedS = true;
  else if (e->keysym.sym == SDLK_d)
    hasPressedD = true;
}

float InitState::miscLoader() {
  initTable(); //Trig tables
  sparkCountMultiplier=1;
  if (conf["conf"]["audio_enabled"]) audio::init();
  prepareTclBridge();
  namegenLoad();
  return 2;
}

float InitState::starLoader() {
  initStarLists();
  return 2;
}

float InitState::backgroundLoader() {
  if (!headless && !loadBackgroundObjects()) exit(EXIT_MALFORMED_DATA);
  return 2;
}

float InitState::systexLoader() {
  if (!headless)
    system_texture::load();
  return 2;
}

float InitState::initFontLoader() {
  float mult = (preliminaryRunMode? 2.0f : min(vheight,1.0f));
  float size = conf["conf"]["hud"]["font_size"];
  new (sysfont)         Font("fonts/westm",   size*mult, false);
  new (sysfontStipple)  Font("fonts/westm",   size*mult, true );
  new (smallFont)       Font("fonts/unifont", size*mult/1.5f, false);
  new (smallFontStipple)Font("fonts/unifont", size*mult/1.5f, true );
  return 2;
}

//Delays for 500 ms to wait for keystrokes
float InitState::keyboardDelay() {
  if (preliminaryRunMode) return 2; //Don't wait

  static Uint32 start = SDL_GetTicks();
  Uint32 end = SDL_GetTicks();
  return (end-start)/512.0f;
}
