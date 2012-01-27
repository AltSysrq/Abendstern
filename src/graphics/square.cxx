/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/square.hxx
 */

/*
 * square.cxx
 *
 *  Created on: 25.01.2011
 *      Author: jason
 */

#include <GL/gl.h>
#include "cmn_shaders.hxx"
#include "square.hxx"
#include "glhelp.hxx"
#include "gl32emu.hxx"

static GLuint vao,vbo;

static const shader::VertTexc vertices[4] = {
    { {{0,0,0,1}}, {{0,1}} },
    { {{1,0,0,1}}, {{1,1}} },
    { {{0,1,0,1}}, {{0,0}} },
    { {{1,1,0,1}}, {{1,0}} },
};

void square_graphic::bind() {
  static bool init=false;
  if (!init) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();
    init=true;
  } else {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
  }
}
