/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/old_style_explosion.hxx
 */

#include <GL/gl.h>
#include <cstdlib>
#include <iostream>

#include "old_style_explosion.hxx"
#include "explosion.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/fasttrig.hxx"
#include "src/globals.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"

using namespace std;

OldStyleExplosion::OldStyleExplosion(const Explosion* ex)
: GameObject(ex->getField(), ex->getX(), ex->getY(), ex->getVX(), ex->getVY()),
#ifndef AB_OPENGL_14
  vao(newVAO()), vbo(newVBO()),
#else
  vao(0), vbo(0),
#endif /* AB_OPENGL_14 */
  r(ex->getColourR()), g(ex->getColourG()), b(ex->getColourB()),
  sizeAt1Sec(ex->getSize()), maxLife(ex->getLifetime()),
  currLife(0)
{
  #ifndef AB_OPENGL_14
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  numPoints = (unsigned)(sizeAt1Sec*sizeAt1Sec*ex->getDensity()*65536);
  shader::quickV* vertices = new shader::quickV[numPoints];
  for (unsigned i=0; i<numPoints; ++i) {
    float r = frand() / (float)RAND_MAX;
    float t = frand() / (float)RAND_MAX * 2 * pi;
    vertices[i].vertex[0] = r*r*fcos(t);
    vertices[i].vertex[1] = r*r*fsin(t);
  }
  glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices)*numPoints, vertices, GL_STATIC_DRAW);
  shader::quick->setupVBO();
  delete[] vertices;
  #endif /* AB_OPENGL_14 */

  includeInCollisionDetection=false;
  decorative = true;
  drawLayer = Background;
  okToDecorate();
}

OldStyleExplosion::~OldStyleExplosion() {
  #ifndef AB_OPENGL_14
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  #endif /* AB_OPENGL_14 */
}

bool OldStyleExplosion::update(float et) noth {
#ifndef AB_OPENGL_14
  x += vx*et;
  y += vy*et;
  currLife += et;
  return currLife < maxLife;
#else
  return false;
#endif /* AB_OPENGL_14 */
}

void OldStyleExplosion::draw() noth {
#ifndef AB_OPENGL_14
  BEGINGP("Old-Style Explosion")
  mPush();
  mTrans(x,y);
  mUScale(currLife * sizeAt1Sec / 1000);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::quickU uni = { {{r, g, b, 1.0f - currLife/maxLife}} };
  shader::quick->activate(&uni);
  glDrawArrays(GL_POINTS, 0, numPoints);
  mPop();
  ENDGP
#endif /* AB_OPENGL_14 */
}

float OldStyleExplosion::getRadius() const noth {
  return sizeAt1Sec * currLife / 1000;
}
