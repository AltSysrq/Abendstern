/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/asgi.hxx
 */

/*
 * asgi.cxx
 *
 *  Created on: 22.01.2011
 *      Author: jason
 */

#include <vector>
#include <exception>
#include <stdexcept>

#include <GL/gl.h>

#include "vaoemu.hxx"
#include "asgi.hxx"
#include "vec.hxx"
#include "mat.hxx"
#include "matops.hxx"
#include "cmn_shaders.hxx"
#include "shader.hxx"
#include "src/globals.hxx"

using namespace std;

#define MAX_STACK_HEIGHT 64
#define MAX_VERTICES 1024

static bool isRendering;
static asgi::Primitive renderingWhat;
static vector<shader::basicV> vertices;
static vec4 currentColour;

asgi::AsgiException::AsgiException(const char* what)
: runtime_error(what) {}

void asgi::begin(asgi::Primitive type) {
  if (isRendering)
    throw AsgiException("Call to asgi::begin during rendering.");

  isRendering=true;
  renderingWhat = type;
  #ifdef AB_OPENGL_14
  glDisable(GL_LINE_STIPPLE);
  glDisable(GL_POLYGON_STIPPLE);
  glDisable(GL_TEXTURE_2D);
  glBegin(type == Points?       GL_POINTS
         :type == Lines?        GL_LINES
         :type == LineStrip?    GL_LINE_STRIP
         :type == LineLoop?     GL_LINE_LOOP
         :type == Triangles?    GL_TRIANGLES
         :type == TriangleStrip?GL_TRIANGLE_STRIP
         :type == TriangleFan?  GL_TRIANGLE_FAN
         :/*type == Quads?*/    GL_QUADS);
  #endif /* AB_OPENGL_14 */
}

void asgi::end() {
  if (!isRendering)
    throw AsgiException("Call to asgi::end outside of rendering.");

  #ifndef AB_OPENGL_14
  if (renderingWhat == asgi::Lines && (vertices.size()&1))
    throw AsgiException("Odd number of vertices for lines.");
  else if ((renderingWhat == asgi::LineStrip || renderingWhat == asgi::LineLoop)
       &&  vertices.size() < 2)
    throw AsgiException("Too few vertices for line strip.");
  else if (renderingWhat == asgi::Triangles && (vertices.size()%3))
    throw AsgiException("Bad number of vertices for triangles.");
  else if ((renderingWhat == asgi::TriangleStrip || renderingWhat == asgi::TriangleFan)
       &&  vertices.size() < 3)
    throw AsgiException("Too few vertices for triangle strip.");
  else if (renderingWhat == asgi::Quads && (vertices.size()&3))
    throw AsgiException("Bad number of vertices for quads.");

  /* If we are drawing quads, we'll be telling OpenGL that they're a triangle
   * strip. To make that work, we need to swap every third and fourth vertex, ie:
   * From:
   *   1        2
   *   .        .
   *   4        3
   * To:
   *   1        2
   *   .        .
   *   3        4
   */
  if (renderingWhat == asgi::Quads) {
    for (unsigned i=0; i<vertices.size(); i += 4) {
      shader::basicV tmp=vertices[i+2];
      vertices[i+2]=vertices[i+3];
      vertices[i+3]=tmp;
    }
    renderingWhat = asgi::TriangleStrip;
  }

  static bool hasAllocatedBOs=false;
  static GLuint vao, vbo;

  if (!hasAllocatedBOs) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    shader::basic->setupVBO();
    hasAllocatedBOs=true;
  }

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(shader::basicV)*vertices.size(), &vertices[0], GL_STREAM_DRAW);
  shader::basic->activate(NULL);
  glDrawArrays(renderingWhat, 0, vertices.size());

  isRendering=false;
  vertices.clear();
  #else /* AB_OPENGL_14 defined */
  glEnd();
  isRendering=false;
  #endif /* AB_OPENGL_14 defined */
}

void asgi::vertex(float x, float y) {
  if (!isRendering)
    throw asgi::AsgiException("Call to vertex outside of rendering.");
  if (vertices.size() >= MAX_VERTICES)
    throw asgi::AsgiException("Vertex limit exceeded.");

  #ifndef AB_OPENGL_14
  shader::basicV vertex;
  vertex.vertex = Vec4(x,y);
  vertex.colour = currentColour;
  vertices.push_back(vertex);
  #else /* defined(AB_OPENGL_14) */
  glVertex2f(x,y);
  #endif /* defined(AB_OPENGL_14) */
}

void asgi::colour(float r, float g, float b, float a) {
  currentColour=Vec4(r,g,b,a);
  #ifdef AB_OPENGL_14
  glColor4fv(currentColour.v);
  #endif
}

void asgi::pushMatrix() {
  if (mSize() >= MAX_STACK_HEIGHT)
    throw asgi::AsgiException("Matrix stack limit exceeded.");
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");

  mPush();
}

void asgi::popMatrix() {
  if (mSize() <= 1)
    throw asgi::AsgiException("Matrix stack empty.");
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mPop();
}

void asgi::loadIdentity() {
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mId();
}

void asgi::translate(float x, float y) {
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mTrans(x,y);
}

void asgi::rotate(float th) {
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mRot(th/180*pi);
}

void asgi::scale(float x, float y) {
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mScale(x,y);
}

void asgi::uscale(float f) {
  if (isRendering)
    throw asgi::AsgiException("Matrix operation during rendering.");
  mUScale(f);
}

void asgi::reset() {
  mClear();
  #ifdef AB_OPENGL_14
  if (isRendering) glEnd();
  #endif
  isRendering=false;
  vertices.clear();
  currentColour = Vec4(1,1,1,1);
}

const vec4& asgi::getColour() {
  return currentColour;
}
