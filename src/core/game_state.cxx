/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/core/game_state.hxx
 */

#include <iostream>

//#include <GL/gl.h>
#include "src/globals.hxx"
#include "game_state.hxx"
#include "src/graphics/matops.hxx"

using namespace std;

void GameState::configureGL() {
  /*
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //Preserve aspect ratio by adjusting height
  glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  */
  mId(matrix_stack::view);
  mTrans(-1, -1, matrix_stack::view);
  mUScale(2, matrix_stack::view);
  mConc(matrix(1,0,0,0,
               0,1/(vheight=screenH/(float)screenW),0,0,
               0,0,1,0,
               0,0,0,1), matrix_stack::view);
  #ifdef AB_OPENGL_14
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  #endif

  mId();
}

GameState::~GameState() {
}
