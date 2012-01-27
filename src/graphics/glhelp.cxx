/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/glhelp.hxx
 */

/*
 * glhelp.cxx
 *
 *  Created on: 23.01.2011
 *      Author: jason
 */

#ifndef GLHELP_CXX_
#define GLHELP_CXX_

#include <GL/gl.h>
#include <cstdlib>
#include <iostream>

#include "glhelp.hxx"
#include "gl32emu.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

GLuint newVAO() {
  GLuint vao=0;
  glGenVertexArrays(1,&vao);
  if (!vao) {
    cerr << "FATAL: Unable to create vertex array object!" << endl;
    exit(EXIT_PLATFORM_ERROR);
  }
  return vao;
}

GLuint newVBO() {
  GLuint vbo=0;
  glGenBuffers(1,&vbo);
  if (!vbo) {
    cerr << "FATAL: Unable to create vertex buffer object!" << endl;
    exit(EXIT_PLATFORM_ERROR);
  }
  return vbo;
}

#endif /* GLHELP_CXX_ */
