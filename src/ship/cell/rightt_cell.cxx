/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/rightt_cell.hxx
 */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <GL/gl.h>

#include "rightt_cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/ship_pallet.hxx"
#include "src/globals.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/exit_conditions.hxx"
using namespace cell_colours;
using namespace std;

#ifndef AB_OPENGL_14
static GLuint backVao, backVbo, backIbo,
              cxVao, cxVbo,
              cirVao, cirVbo,
              damVao, damVbo;

/* Similar system as for square cells. See full explanations
 * in square_cell.cxx. The only difference is that we do not
 * have a centre vertex, since that is the same as the long
 * side.
 *
 * Our first vertex is the centre of the hypoteneuse. (we have the shape /| ).
 */
#define DK P_IX2F(P_BODY_BEGIN)
#define LT P_IX2F(P_BODY_END)
#define VERT(x,y,l) { {{x,y,0,1}}, {{l,l,l,1}} }
#define VERTP(x,y) VERT(x,y,DK), VERT(x,y,LT)
static const shader::VertCol backVerts[] = {
  VERTP(+0,+0),
  VERTP(-1,-1),
  VERTP(+0,-1),
  VERTP(+1,-1),
  VERTP(+1,+0),
  VERTP(+1,+1),
};
#undef DK
#undef LT
#undef VERTP

static const shader::VertTexc damVerts[3] = {
  { {{-1,-1,0,1}}, {{0,0}} },
  { {{+1,-1,0,1}}, {{1,0}} },
  { {{+1,+1,0,1}}, {{1,1}} },
};

#define VERTH(x,y) VERT((x)/8.0f,(y)/8.0f,P_IX2F((P_BODY_END+P_BODY_BEGIN)/2))
static const shader::VertCol cxnVerts[12] = {
  VERTH(+5.0f,-5.0f), VERTH(+8.0f, -2.0f), VERTH(+3.0f, -3.0f), VERTH(+8.0f, +2.0f),
  VERTH(-sqrt(2.0f),-sqrt(2.0f)), VERTH(5-sqrt(2.0f), -5-sqrt(2.0f)),
  VERTH(+sqrt(2.0f),+sqrt(2.0f)), VERTH(5+sqrt(2.0f), -5+sqrt(2.0f)),
  VERTH(-2.0f, -8.0f), VERTH(+2.0f, -8.0f), VERTH(+3.0f,-3.0f), VERTH(+5.0f,-5.0f),
};
#undef VERTH
#undef VERT

/* The circuits for a right triangle are a bit different. Mainly,
 * there is no centre, but there are single connections.
 */
#define LINE(x1,y1,x2,y2) {{{(x1)/8.0f,(y1)/8.0f}}}, {{{(x2)/8.0f,(y2)/8.0f}}}
#define SQ sqrt(2.0f)
static const shader::Vert2 cirVerts[] = {
  //BEGIN: NEIGHBOUR
  LINE(+8.0f, +3.2f, +2.4f, -2.4f), //0, 0
  LINE(+8.0f, -3.2f, +5.6f, -5.6f), //2, 0
  LINE(+5.6f, -5.6f, +2.4f, -2.4f), //4, 0
  LINE(+1.6f*SQ,+1.6f*SQ, +2.6f*SQ,+0.6f*SQ), //6, 1
  LINE(-1.6f*SQ,-1.6f*SQ, -0.6f*SQ,-2.6f*SQ), //8, 1
  LINE(+2.6f*SQ,+0.6f*SQ, -0.6f*SQ,-2.6f*SQ), //10, 1
  LINE(+3.2f, -8.0f, +5.6f, -5.6f), //12, 2
  LINE(-3.2f, -8.0f, +2.4f, -2.4f), //14, 4
  LINE(+5.6f, -5.6f, +2.4f, -2.4f),
  //END: NEIGHBOUR
};
#undef SQ
#undef LINE
#define CIRCUIT_NEIGHBOUR_BEGIN 0
#define CIRCUIT_NEIGHBOUR_SZ 6

/* Index bitmask:
 * 0    COLOURE(0)
 * 1    COLOURE(1)
 * 2    COLOURE(2)
 * 3    COLOURC(0,1)
 * 4    COLOURC(1,2)
 * 5    COLOURC(2,0)
 */
#define INDEX_BLOCK_SZ 6
#define INDICES(x) \
  ((x)&(1<<1)?1:0) + 0,\
  ((x)&(1<<4)?1:0) + 2,\
  ((x)&(1<<2)?1:0) + 4,\
  ((x)&(1<<5)?1:0) + 6,\
  ((x)&(1<<0)?1:0) + 8,\
  ((x)&(1<<3)?1:0) + 10
#define INDICES2(x) INDICES(x), INDICES(x+1)
#define INDICES4(x) INDICES2(x), INDICES2(x+2)
#define INDICES8(x) INDICES4(x), INDICES4(x+4)
#define INDICES10(x) INDICES8(x), INDICES8(x+8)
#define INDICES20(x) INDICES10(x), INDICES10(x+16)
static const GLubyte backIxs[64*INDEX_BLOCK_SZ] = {
  INDICES20(0), INDICES20(32),
};
#undef INDICES20
#undef INDICES10
#undef INDICES8
#undef INDICES4
#undef INDICES2
#undef INDICES

#else /* defined(AB_OPENGL_14) */
struct rt_circuit {
  bool neighbours[3];
  GLfloat vertices[6][2];
  int count;
};

rt_circuit rt_circuits[] = {
  { { true, false, false },
    { { +8.0f*PIX, +5.0f*PIX },
      { +1.5f*PIX, -1.5f*PIX },
      { +8.0f*PIX, -5.0f*PIX },
      { +6.5f*PIX, -6.5f*PIX },
      { +6.5f*PIX, -6.5f*PIX },
      { +1.5f*PIX, -1.5f*PIX },
    }, 6
  },
  { { false, false, true },
    { { +5.0f*PIX, -8.0f*PIX },
      { +6.5f*PIX, -6.5f*PIX },
      { -5.0f*PIX, -8.0f*PIX },
      { +1.5f*PIX, -1.5f*PIX },
      { +6.5f*PIX, -6.5f*PIX },
      { +1.5f*PIX, -1.5f*PIX },
    }, 6
  },
  { { false, true, false },
    { { +2.5f*sqrt(2.0f)*PIX, +2.5f*sqrt(2.0f)*PIX },
      { +3.5f*sqrt(2.0f)*PIX, +1.5f*sqrt(2.0f)*PIX },
      { -2.5f*sqrt(2.0f)*PIX, -2.5f*sqrt(2.0f)*PIX },
      { -1.5f*sqrt(2.0f)*PIX, -3.5f*sqrt(2.0f)*PIX },
      { +3.5f*sqrt(2.0f)*PIX, +1.5f*sqrt(2.0f)*PIX },
      { -1.5f*sqrt(2.0f)*PIX, -3.5f*sqrt(2.0f)*PIX },
    }, 6
  },
  { { true, true, false },
    { { +8.0f*PIX, -5.0f*PIX },
      { +3.5f*sqrt(2.0f)*PIX, +1.5f*sqrt(2.0f)*PIX },
    }, 2
  },
  { { true, false, true },
    { { -5.0f*PIX, -8.0f*PIX },
      { -1.5f*sqrt(2.0f)*PIX, -3.5f*sqrt(2.0f)*PIX },
    }, 2
  },
};

namespace rightt_verts {
  float vertices[3][2] = {
    { +STD_CELL_SZ/2, -STD_CELL_SZ/2 },
    { +STD_CELL_SZ/2, +STD_CELL_SZ/2 },
    { -STD_CELL_SZ/2, -STD_CELL_SZ/2 },
  };
  //The other "vertices" for gradients; the midpoints of the sides
  #define AVG(a,b) { (vertices[a][0]+vertices[b][0])/2, (vertices[a][1]+vertices[b][1])/2 }
  float altverts[3][2] = {
    AVG(0,1), AVG(1,2), AVG(2,0)
  };
  #undef AVG
  float centre[2] = { (vertices[0][0]+vertices[1][0]+vertices[2][0])/3,
                      (vertices[0][1]+vertices[1][1]+vertices[2][1])/3 };
};
using namespace rightt_verts;

#endif /* AB_OPENGL_14 */

RightTCell::RightTCell(Ship* parent)
: Cell(parent), hasCalculatedColourInfo(false)
{
  _intrinsicMass=35;

#ifndef AB_OPENGL_14
  static bool hasVao=false;
  if (!hasVao && !headless) {
    backVao=newVAO();
    glBindVertexArray(backVao);
    backVbo=newVBO();
    backIbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, backVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backIbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backVerts), backVerts, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(backIxs), backIxs, GL_STATIC_DRAW);
    shader::basic->setupVBO();

    cxVao=newVAO();
    glBindVertexArray(cxVao);
    cxVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, cxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cxnVerts), cxnVerts, GL_STATIC_DRAW);
    shader::basic->setupVBO();

    cirVao=newVAO();
    glBindVertexArray(cirVao);
    cirVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, cirVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cirVerts), cirVerts, GL_STATIC_DRAW);
    shader::quick->setupVBO();

    damVao=newVAO();
    glBindVertexArray(damVao);
    damVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER,damVbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(damVerts),damVerts, GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();

    hasVao=true;
  }
#endif /* AB_OPENGL_14 */
}

unsigned RightTCell::numNeighbours() const noth {
  return 3;
}

float RightTCell::edgeD(int n) const noth {
  //The "centre" is actually the very middle of the hypoteneuse.
  //This actually simplifies the math greatly
  switch(n) {
    case 0:
    case 2: return STD_CELL_SZ/2;
    case 1: return 0;
    default:
      cerr << "Unexpected value to RightTCell::edgeD(int): " << n << endl;
      exit(EXIT_PROGRAM_BUG);
  }
  return 0; //So the compiler won't complain
}

int RightTCell::edgeT(int n) const  noth{
  switch(n) {
    case 0: return 0;
    //This might not seem meaningful, since the distance is zero, but
    //it comes into play for cell rotation
    case 1: return 90+45;
    case 2: return 270;
    default:
      cerr << "Unexpected value to RightTCell::edgeT(int): " << n << endl;
      exit(EXIT_PROGRAM_BUG);
  }
  return 0;
}

int RightTCell::edgeDT(int n) const noth {
  switch(n) {
    case 0:
    case 2: return 0;
    case 1: return 45;
    default:
      cerr << "Unexpected value to RightTCell::edgeDT(int): " << n << endl;
      exit(EXIT_PROGRAM_BUG);
  }
  return 0;
}

float RightTCell::getCentreX() const noth {
  return +STD_CELL_SZ/4;
}

float RightTCell::getCentreY() const noth {
  return -STD_CELL_SZ/4;
}

void RightTCell::drawThis() noth {
  BEGINGP("RightTCell")

#ifndef AB_OPENGL_14
  mUScale(STD_CELL_SZ/2);

  if (!hasCalculatedColourInfo) {
    COLOURC_PREP

    ixIx = COLOURE(0)
         | (COLOURE(1) << 1)
         | (COLOURE(2) << 2)
         | (COLOURC(0,1) << 3)
         | (COLOURC(1,2) << 4)
         | (COLOURC(2,0) << 5);
    hasCalculatedColourInfo=true;
  }

  glBindVertexArray(backVao);
  glBindBuffer(GL_ARRAY_BUFFER, backVbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backIbo);
  shader::basic->activate(NULL);
  glDrawElements(GL_TRIANGLE_FAN, INDEX_BLOCK_SZ, GL_UNSIGNED_BYTE,
                 reinterpret_cast<const GLvoid*>(ixIx*INDEX_BLOCK_SZ));

  glBindVertexArray(cxVao);
  glBindBuffer(GL_ARRAY_BUFFER, cxVbo);
  for (unsigned i=0; i<3; ++i)
    if (neighbours[i])
      glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);

  shader::Colour c = {{{P_IX2F(P_BLACK),P_IX2F(P_BLACK),P_IX2F(P_BLACK),1}}};
  glBindVertexArray(cirVao);
  glBindBuffer(GL_ARRAY_BUFFER, cirVbo);
  shader::quick->activate(&c);
  for (unsigned i=0; i<3; ++i)
    if (neighbours[i])
      glDrawArrays(GL_LINES, CIRCUIT_NEIGHBOUR_BEGIN + i*CIRCUIT_NEIGHBOUR_SZ, CIRCUIT_NEIGHBOUR_SZ);
#else /* defined(AB_OPENGL_14) */
  glBegin(GL_TRIANGLES);
  bool gradient=true;
  if (!gradient) {
    for (int i=0; i<3; ++i) glVertex2fv(vertices[i]);
  } else {
    COLOURC_PREP
    for (int i=0; i<3; ++i) {
      parent->glSetColour();
      glVertex2fv(centre);
      COLOURC((i==0? 2:i-1),i);
      glVertex2fv(vertices[i]);
      COLOURE(i);
      glVertex2fv(altverts[i]);

      parent->glSetColour();
      glVertex2fv(centre);
      COLOURE(i);
      glVertex2fv(altverts[i]);
      COLOURC(i,(i+1)%3);
      glVertex2fv(vertices[(i+1)%3]);
    }
  }
  glEnd();

  parent->glSetColour(0.5f);
  glBegin(GL_QUADS);
  if (neighbours[0]) {
    glVertex2f(+8.0f*PIX, +2.0f*PIX);
    glVertex2f(+3.0f*PIX, -3.0f*PIX);
    glVertex2f(+5.0f*PIX, -5.0f*PIX);
    glVertex2f(+8.0f*PIX, -2.0f*PIX);
  }
  if (neighbours[1]) {
    glVertex2f(-sqrt(2.0f)*PIX, -sqrt(2.0f)*PIX);
    glVertex2f((+5-sqrt(2.0f))*PIX, (-5-sqrt(2.0f))*PIX);
    glVertex2f((+5+sqrt(2.0f))*PIX, (-5+sqrt(2.0f))*PIX);
    glVertex2f(+sqrt(2.0f)*PIX, +sqrt(2.0f)*PIX);
  }
  if (neighbours[2]) {
    glVertex2f(+2.0f*PIX, -8.0f*PIX);
    glVertex2f(+5.0f*PIX, -5.0f*PIX);
    glVertex2f(+3.0f*PIX, -3.0f*PIX);
    glVertex2f(-2.0f*PIX, -8.0f*PIX);
  }
  glEnd();

  if (generalAlphaBlending)
    glColor4fv(semiblack);
  else
    glColor3fv(semiblack);
  glBegin(GL_LINES);
  for (unsigned int i=0; i<sizeof(rt_circuits)/sizeof(rt_circuit); ++i) {
    bool ok=true;
    for (int n=0; n<3; ++n)
      if (rt_circuits[i].neighbours[n])
        ok &= (neighbours[n]!=NULL);

    if (ok)
      for (int v=0; v<rt_circuits[i].count; ++v)
        glVertex2fv(rt_circuits[i].vertices[v]);
  }
  glEnd();
  mUScale(STD_CELL_SZ/2);
#endif /* AB_OPENGL_14 */

  if (systems[0]) {
    mTrans(+0.5,-0.5);
    mUScale(2); //Cancel out the STD_CELL_SZ/2
    mRot(-pi/2);
    //Nudge to the right if nothing special
    if (systems[0]->positioning == ShipSystem::Centre
    ||  systems[0]->size == ShipSystem::Large) {
      //No movement
      if (0 != systems[0]->getAutoRotate())
        mRot(systems[0]->getAutoRotate());
    } else if (systems[0]->positioning == ShipSystem::Forward) {
      //Forward slightly
      if (0 != systems[0]->getAutoRotate())
        mRot(systems[0]->getAutoRotate());
      mTrans(0,0.2f);
    } else if (systems[0]->positioning == ShipSystem::Backward) {
      //Back slightly
      if (0 != systems[0]->getAutoRotate())
        mRot(systems[0]->getAutoRotate());
      mTrans(0,-0.2f);
    } else {
      if (0 != systems[0]->getAutoRotate())
        mRot(systems[0]->getAutoRotate());
    }
    mTrans(-0.5f,-0.5f);
    systems[0]->draw();
  }
  ENDGP
}

void RightTCell::drawShapeThis(float,float,float,float) noth {
  BEGINGP("RightTCell::drawShapeThis")
  ENDGP
}

void RightTCell::drawDamageThis() noth {
#ifndef AB_OPENGL_14
  mUScale(STD_CELL_SZ/2);
  glBindVertexArray(damVao);
  glBindBuffer(GL_ARRAY_BUFFER,damVbo);
  shader::textureReplaceU uni;
  uni.colourMap=0;
  shader::textureReplace->activate(&uni);
  glDrawArrays(GL_TRIANGLES,0,3);
#else
  glBegin(GL_TRIANGLES);
  for (int i=0; i<3; ++i) {
    glTexCoord2f(vertices[i][0]/STD_CELL_SZ+0.5f, vertices[i][1]/STD_CELL_SZ+0.5f);
    glVertex2fv(vertices[i]);
  }
  glEnd();
#endif /* AB_OPENGL_14 */
}

CollisionRectangle* RightTCell::getCollisionBounds() noth {
  pair<float,float> coords[4] = {
    Ship::cellCoord(parent, this, +8*PIX, +8*PIX),
    Ship::cellCoord(parent, this, +8*PIX, -8*PIX),
    Ship::cellCoord(parent, this, -8*PIX, -8*PIX),
    Ship::cellCoord(parent, this),
  };
  for (int i=0; i<4; ++i)
    collisionRectangle.vertices[i]=coords[i];

  return &collisionRectangle;
}

float RightTCell::getIntrinsicDamage() const noth {
  return 11;
}
