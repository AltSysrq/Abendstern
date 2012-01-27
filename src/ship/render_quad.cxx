/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/render_quad.hxx
 */

/*
 * render_quad.cxx
 *
 *  Created on: 30.01.2011
 *      Author: jason
 */

//Only compile in non-compatibility mode
#ifndef AB_OPENGL_14

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include <GL/gl.h>

#include "render_quad.hxx"
#include "cell/cell.hxx"
#include "src/sim/collision.hxx"
#include "src/globals.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

#define SHIP_TEX_SZ 256
#define DAM_TEX_SZ  64

RenderQuad::RenderQuad(unsigned detail, unsigned x, unsigned y)
: hasVao(false), hasTex(false), shipTexValid(false), damTexValid(false),
  detailLevel(detail),
  minX(detail*STD_CELL_SZ*x), minY(detail*STD_CELL_SZ*y),
  maxX(detail*STD_CELL_SZ*(x+1)), maxY(detail*STD_CELL_SZ*(y+1))
{
}

RenderQuad::~RenderQuad() {
  free();
}

void RenderQuad::free() noth {
  if (hasVao) {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    hasVao=false;
  }

  if (hasTex) {
    glDeleteTextures(1, &shipTex);
    glDeleteTextures(1, &damTex);
    hasTex=false;
  }
}

bool RenderQuad::doesCellIntersect(Cell* cell) noth {
  const CollisionRectangle* rectPtr=cell->getCollisionBounds();
  if (!rectPtr) return false;
  const CollisionRectangle& crect(*rectPtr);

  bool hit = false;
  for (unsigned i=0; i<4 && !hit; ++i)
    hit = (crect.vertices[i].first  >= minX-STD_CELL_SZ
        && crect.vertices[i].first  <= maxX+STD_CELL_SZ
        && crect.vertices[i].second >= minY-STD_CELL_SZ
        && crect.vertices[i].second <= maxY+STD_CELL_SZ);

  if (hit) cells.push_back(cell);
  return hit;
}

void RenderQuad::cellDamaged() noth {
  damTexValid=false;
}

void RenderQuad::physicalChange(const Cell* destroyed) noth {
  damTexValid=shipTexValid=false;
  if (destroyed) {
    cells.erase(find(cells.begin(), cells.end(), destroyed));
    if (cells.empty())
      free();
  }
}

void RenderQuad::makeReady(float tx, float ty) noth {
  if (!hasVao) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const shader::VertTexc vertices[4] = {
      { {{minX,minY,0,1}}, {{0,0}} },
      { {{maxX,minY,0,1}}, {{1,0}} },
      { {{minX,maxY,0,1}}, {{0,1}} },
      { {{maxX,maxY,0,1}}, {{1,1}} },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();

    hasVao=true;
  }

  if (!hasTex) {
    glGenTextures(1, &shipTex);
    glGenTextures(1, &damTex);

    glBindTexture(GL_TEXTURE_2D, shipTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, damTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    hasTex=true;
    damTexValid=shipTexValid=false;
  }

  static GLuint fbo=0;
  if (!fbo) {
    glGenFramebuffers(1, &fbo);
  }

  if (!shipTexValid) {
    mPush();
    mUScale(SHIP_TEX_SZ/2.0f/detailLevel);
    mTrans(tx-minX,ty-minY);

    static const GLubyte initTexData[SHIP_TEX_SZ*SHIP_TEX_SZ] = {0};

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_2D, shipTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, SHIP_TEX_SZ, SHIP_TEX_SZ, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, initTexData);
    glViewport(0,0,SHIP_TEX_SZ, SHIP_TEX_SZ);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shipTex, 0);

    switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)) {
      #define ERR(name) case name: cerr << "FATAL: Unable to set framebuffer: " #name << endl; exit(EXIT_PLATFORM_ERROR)
      ERR(GL_FRAMEBUFFER_UNDEFINED);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
      ERR(GL_FRAMEBUFFER_UNSUPPORTED);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
      case GL_FRAMEBUFFER_COMPLETE: break; //OK
      default:
        cerr << "Unknown error setting framebuffer!" << endl;
        exit(EXIT_PLATFORM_ERROR);
      #undef ERR
    }

    for (unsigned i=0; i<cells.size(); ++i)
      cells[i]->draw();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    mPop();
    glViewport(0,0,screenW,screenH);

    shipTexValid=true;
  }

  if (!damTexValid) {
    mPush();
    mUScale(DAM_TEX_SZ*2.0f/detailLevel);
    mTrans(tx-minX,ty-minY);

    static GLubyte initTexData[DAM_TEX_SZ*DAM_TEX_SZ];
    memset(initTexData, 255, sizeof(initTexData));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_2D, damTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, DAM_TEX_SZ, DAM_TEX_SZ, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, initTexData);
    glViewport(0,0,DAM_TEX_SZ, DAM_TEX_SZ);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, damTex, 0);

    switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)) {
      #define ERR(name) case name: cerr << "FATAL: Unable to set framebuffer: " #name << endl; exit(EXIT_PLATFORM_ERROR)
      ERR(GL_FRAMEBUFFER_UNDEFINED);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
      ERR(GL_FRAMEBUFFER_UNSUPPORTED);
      ERR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
      case GL_FRAMEBUFFER_COMPLETE: break; //OK
      default:
        cerr << "Unknown error setting framebuffer!" << endl;
        exit(EXIT_PLATFORM_ERROR);
      #undef ERR
    }

    for (unsigned i=0; i<cells.size(); ++i)
      cells[i]->drawDamage();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    mPop();
    glViewport(0,0,screenW,screenH);
    damTexValid=true;
  }
}

void RenderQuad::draw() const noth {
  if (cells.empty()) return;

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindTexture(GL_TEXTURE_2D,shipTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,damTex);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glBindTexture(GL_TEXTURE_2D,0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,0);
}

#endif /* AB_OPENGL_14 */
