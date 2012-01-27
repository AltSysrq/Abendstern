/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/secondary/light_trail.hxx
 */

/*
 * light_trail.cxx
 *
 *  Created on: 11.02.2011
 *      Author: jason
 */

#include <algorithm>
#include <vector>
#include <iostream>

#include "light_trail.hxx"
#include "src/sim/game_field.hxx"
#include "src/globals.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/cmn_shaders.hxx"

using namespace std;

#define NO_DRAW_LIFE 5000
#define BUFFER_SPACE 4

#define UNIFORM_TYPE LightTrail::Uniform
#define VERTEX_TYPE LightTrail::Vertex
#ifndef AB_OPENGL_14

/**
 * The shader stack will be emulated in 2.1 mode by adding each
 * vertex twice (with ix being positive and negative one) and
 * letting the vertex shader expand the two (also given the
 * velocity at that point to determine which way to expand).
 */
DELAY_SHADER(trail)
  sizeof(LightTrail::Vertex),
  #ifdef AB_OPENGL_21
  VFLOAT(ix),
  #endif
  VATTRIB(vertex), VATTRIB(velocity),
  VFLOAT(creation), VFLOAT(isolation), VFLOAT(expansion),
  NULL,
  true,
  UNIFORM(baseColour), UNIFORM(colourFade),
  UNIFLOAT(currentTime), UNIFLOAT(baseWidth),
  NULL
END_DELAY_SHADER(static trailShader);

#ifdef AB_OPENGL_21
#define VM 2
#else
#define VM 1
#endif
#endif /* AB_OPENGL_14 */


LightTrail::LightTrail(GameField* field, float ttl, unsigned res,
                       float maxw, float basew,
                       float br, float bg, float bb, float ba,
                       float fr, float fg, float fb, float fa)
: GameObject(field),
  resolution(res+BUFFER_SPACE),
  creationTime(field->fieldClock),
  lifeTime(ttl),
  nextVertexStatus(None), timeBetweenVertexAdd(ttl/res),
  timeUntilVertexAdd(0), currentVertex(0),
  timeSinceDraw(0)
{
  #ifndef AB_OPENGL_14
  uniform.baseColour = Vec4(br,bg,bb,ba);
  uniform.colourFade = Vec4(fr/ttl,fg/ttl,fb/ttl,fa/ttl);
  nextVertex.expansion = maxw/ttl/2;
  uniform.baseWidth = basew/2;
  usedVertices.assign(resolution, 0);

  vao=newVAO();
  glBindVertexArray(vao);
  vbo=newVBO();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  vio=newVBO();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
  //Leave the buffer uninitialized, we'll specify data when we have it
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*VM*resolution, NULL, GL_DYNAMIC_DRAW);

  unsigned short* indices=new unsigned short[resolution*2*VM];
  for (unsigned i=0; i<resolution*VM; ++i)
    indices[i] = indices[i+resolution*VM] = i;
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices)*2*VM*resolution,
               indices, GL_STATIC_DRAW);
  delete[] indices;

  trailShader->setupVBO();
  #endif /* AB_OPENGL_14 */

  includeInCollisionDetection=false;
  decorative=true;
  drawLayer=Background;
  okToDecorate();
}

LightTrail::~LightTrail() {
  #ifndef AB_OPENGL_14
  glDeleteBuffers(1, &vio);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  #endif /* AB_OPENGL_14 */
}

float LightTrail::getRadius() const noth {
  return 1;
}

bool LightTrail::update(float et) noth {
  timeSinceDraw += et;
  if (timeSinceDraw > NO_DRAW_LIFE)
    //We no longer have a purpose in life, die
    return false;

  x += vx*et;
  y += vy*et;

  #ifndef AB_OPENGL_14
  timeUntilVertexAdd -= et;
  if (timeUntilVertexAdd < 0) {
    //Next vertex
    bool hasPrevVertex = usedVertices[currentVertex];
    ++currentVertex;
    currentVertex %= resolution;
    unsigned prevVertex = currentVertex + BUFFER_SPACE;
    prevVertex %= resolution;

    if (nextVertexStatus != None) {
      nextVertex.creation = field->fieldClock - creationTime;
      nextVertex.isolation = (nextVertexStatus == Isolated || !hasPrevVertex? 0 : 1);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      #ifdef AB_OPENGL_21
      nextVertex.ix = -1;
      #endif
      glBufferSubData(GL_ARRAY_BUFFER, currentVertex*VM * sizeof(Vertex),
                      sizeof(Vertex), &nextVertex);
      //Add extra vertex in 2.1 mode
      #ifdef AB_OPENGL_21
      nextVertex.ix = +1;
      glBufferSubData(GL_ARRAY_BUFFER, (currentVertex*VM+1) * sizeof(Vertex),
                      sizeof(Vertex), &nextVertex);
      #endif
      usedVertices[currentVertex] = 1;
      //Set the next one to the same thing, time-shifted a little
      nextVertex.creation += timeBetweenVertexAdd;
      #ifdef AB_OPENGL_21
      nextVertex.ix = -1;
      #endif
      glBufferSubData(GL_ARRAY_BUFFER, (currentVertex+1)%resolution*VM * sizeof(Vertex),
                      sizeof(Vertex), &nextVertex);
      //Add extra vertex in 2.1 mode
      #ifdef AB_OPENGL_21
      nextVertex.ix = +1;
      glBufferSubData(GL_ARRAY_BUFFER, ((currentVertex+1)%resolution*VM+1) * sizeof(Vertex),
                      sizeof(Vertex), &nextVertex);
      #endif
      usedVertices[(currentVertex+1)%resolution] = 1;
      nextVertexStatus = (nextVertexStatus == Complete? Isolated : None);
    }
    usedVertices[prevVertex] = 0;

    timeUntilVertexAdd = timeBetweenVertexAdd;
  }
  #endif /* AB_OPENGL_14 */

  return true;
}

void LightTrail::draw() noth {
  #ifndef AB_OPENGL_14
  BEGINGP("LightTrail")
  /* Only count this as drawing if there is actually something to draw. */
  bool hasDrawnAnything = false;

  const unsigned init((currentVertex+BUFFER_SPACE-1)%resolution);
  for (unsigned i=init; i<init+resolution-1; ++i) {
    if (usedVertices[i%resolution]) {
      //Found something to draw
      unsigned begin=i;
      while (i<init+resolution && usedVertices[i%resolution]) ++i;
      unsigned end=i;

      if (!hasDrawnAnything) {
        hasDrawnAnything=true;
        timeSinceDraw=0;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);

        uniform.currentTime = field->fieldClock - creationTime;
        trailShader->activate(&uniform);
      }

      #ifndef AB_OPENGL_21
      glDrawElements(GL_LINE_STRIP_ADJACENCY, end-begin, GL_UNSIGNED_SHORT,
                     reinterpret_cast<const GLvoid*>(begin*sizeof(unsigned short)));
      #else
      begin += VM;
      glDrawElements(GL_TRIANGLE_STRIP, end*VM-begin*VM, GL_UNSIGNED_SHORT,
                     reinterpret_cast<const GLvoid*>(begin*VM*sizeof(unsigned short)));
      #endif
    }
  }
  ENDGP
  #endif /* AB_OPENGL_14 */
}

void LightTrail::emit(float x, float y, float vx, float vy) noth {
  this->x=x;
  this->y=y;
  this->vx=vx;
  this->vy=vy;

  nextVertexStatus=Complete;
  nextVertex.vertex = Vec2(x,y);
  nextVertex.velocity = Vec2(vx,vy);
}

void LightTrail::setWidth(float w) noth {
  nextVertex.expansion = w/2/lifeTime;
}
