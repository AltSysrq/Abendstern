/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/square_cell.hxx
 */

#include <GL/gl.h>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "square_cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/square.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/ship/ship_pallet.hxx"

using namespace cell_colours;
using namespace std;

#ifndef AB_OPENGL_14
static GLuint backVao, backVbo, backIbo,
              cxVao, cxVbo,
              cirVao, cirVbo,
              damVao, damVbo;

/* We'll assume a 16x16 tile (that's what we are at 1024 wide).
 * For a systems unit, draw this pattern:
 *   /----------------\
 *   |   * *    * *   |
 *   |  *  *    *  *  |
 *   | *   *    *   * |
 *   |*    *    *    *|
 *   |     * XX *     |
 *   |     * XX *     |
 *   |****** __ ******|
 *   |      |\/|      |
 *   |      |/\|      |
 *   |****** -- ******|
 *   |     * XX *     |
 *   |     * XX *     |
 *   |*    *    *    *|
 *   | *   *    *   * |
 *   |  *  *    *  *  |
 *   |   * *    * *   |
 *   \----------------/
 */

/* All possible vertices are defined first (we'll assume a
 * [-1,-1]..[+1,+1] coordinate system). We then use an index
 * array to access them.
 * Vertices will be defined in counterclockwise order, starting
 * with the top-right, and will alternate between dark and light.
 * Vertex 0 is the centre, though, and only has a light form.
 */
#define DK P_IX2F(P_BODY_BEGIN)
#define LT P_IX2F(P_BODY_END)
#define VERT(x,y,l) { {{x,y,0,1}}, {{l,l,l,1}} }
#define VERTP(x,y) VERT(x,y,DK), VERT(x,y,LT)
static const shader::VertCol backVerts[] = {
  VERT(0,0,LT),
  VERTP(+1,+1),
  VERTP(+0,+1),
  VERTP(-1,+1),
  VERTP(-1,+0),
  VERTP(-1,-1),
  VERTP(+0,-1),
  VERTP(+1,-1),
  VERTP(+1,+0),
};
#undef VERTP
#undef LT
#undef DK

static const shader::VertTexc damVerts[4] = {
  { {{-1,-1,0,1}}, {{0,0}} },
  { {{+1,-1,0,1}}, {{1,0}} },
  { {{-1,+1,0,1}}, {{0,1}} },
  { {{+1,+1,0,1}}, {{1,1}} },
};

/* The darker-than-normal rectangles that connect the midpoints
 * of all cells. Each is to be drawn with TRIANGLE_STRIP.
 * Indices are in groups of 4, in order of neighbours.
 */
#define VERTH(x,y) VERT(x,y,P_IX2F((P_BODY_END+P_BODY_BEGIN)/2))
static const shader::VertCol cxnVerts[16] = {
  VERTH(0,-0.2f), VERTH(1,-0.2f), VERTH(0,+0.2f), VERTH(1,+0.2f),
  VERTH(-0.2f,0), VERTH(+0.2f,0), VERTH(-0.2f,1), VERTH(+0.2f,1),
  VERTH(0,-0.2f), VERTH(0,+0.2f), VERTH(-1,-0.2f), VERTH(-1,+0.2f),
  VERTH(-0.2f,0), VERTH(-0.2f,-1), VERTH(+0.2f,0), VERTH(+0.2f,-1),
};
#undef VERTH
#undef VERT

/* The "circuits" that cover the main body.
 * The array is divided into several sections:
 *   CIRCUIT_CENTRE_BEGIN+CIRCUIT_CENTRE_SZ
 *     All cells have this, it is the centre criss-cross.
 *   CIRCUIT_NEIGHBOUR_BEGIN+x*CIRCUIT_NEIGHBOUR_SZ
 *     There is one of these for each neighbour, drawn
 *     only if that neighbour exists.
 *   CIRCUIT_DUAL_NEIGHBOUR_BEGIN+x*CIRCUIT_DUAL_NEIGHBOUR_SZ
 *     Similar to normal neighbour circuits, but only drawn
 *     if BOTH neighbours x and (x+1)&3 exist.
 *   CIRCUIT_CIRCLE+CIRCUIT_CIRCLE_SZ
 *     Overlay for circles.
 * All should be drawn with GL_LINES.
 */
#define LINE(x1,y1,x2,y2) {{{x1,y1}}}, {{{x2,y2}}}
#define SPOKE(deg) LINE(0,0,cos(deg*pi/180),sin(deg*pi/180)),\
                   LINE(cos(deg*pi/180),sin(deg*pi/180),cos((deg+30)*pi/180),sin((deg+30)*pi/180))
static const shader::Vert2 cirVerts[] = {
  //BEGIN: CENTRE
  LINE(+0.4f,+0.4f,+0.4f,-0.4f), //0
  LINE(+0.4f,-0.4f,-0.4f,-0.4f), //2
  LINE(-0.4f,-0.4f,-0.4f,+0.4f), //4
  LINE(-0.4f,+0.4f,+0.4f,+0.4f), //6
  LINE(+0.4f,+0.4f,-0.4f,-0.4f), //8
  LINE(+0.4f,-0.4f,-0.4f,+0.4f), //10
  //END: CENTRE
  //BEGIN: NEIGHBOUR
  LINE(+0.4f,+0.4f,+1.0f,+0.4f), //12,0
  LINE(+0.4f,-0.4f,+1.0f,-0.4f), //14,0
  LINE(-0.4f,+0.4f,-0.4f,+1.0f), //16,1
  LINE(+0.4f,+0.4f,+0.4f,+1.0f), //18,1
  LINE(-0.4f,+0.4f,-1.0f,+0.4f), //20,2
  LINE(-0.4f,-0.4f,-1.0f,-0.4f), //22,2
  LINE(-0.4f,-0.4f,-0.4f,-1.0f), //24,3
  LINE(+0.4f,-0.4f,+0.4f,-1.0f), //26,3
  //END: NEIGHBOUR
  //BEGIN: DUAL NEIGHBOUR
  LINE(+1.0f,+0.4f,+0.4f,+1.0f), //28,0+1
  LINE(-0.4f,+1.0f,-1.0f,+0.4f), //30,1+2
  LINE(-1.0f,-0.4f,-0.4f,-1.0f), //32,2+3
  LINE(+0.4f,-1.0f,+1.0f,-0.4f), //34,3+0
  //END: DUAL NEIGHBOUR
  //BEGIN: CIRCLE
  SPOKE(0  ), //36
  SPOKE(30 ), //40
  SPOKE(60 ), //44
  SPOKE(90 ), //48
  SPOKE(120), //52
  SPOKE(150), //56
  SPOKE(180), //60
  SPOKE(210), //64
  SPOKE(240), //68
  SPOKE(270), //72
  SPOKE(300), //76
  SPOKE(330), //80
};
#define CIRCUIT_CENTRE_BEGIN 0
#define CIRCUIT_CENTRE_SZ 12
#define CIRCUIT_NEIGHBOUR_BEGIN 12
#define CIRCUIT_NEIGHBOUR_SZ 4
#define CIRCUIT_DUAL_NEIGHBOUR_BEGIN 28
#define CIRCUIT_DUAL_NEIGHBOUR_SZ 2
#define CIRCUIT_CIRCLE_BEGIN 36
#define CIRCUIT_CIRCLE_SZ 48
#undef LINE
#undef SPOKE

/* The indices are in blocks of INDEX_BLOCK_SZ. That is,
 * The nth form is found by drawing n*INDEX_BLOCK_SZ through
 * (n+1)*INDEX_BLOCK_SZ-1. The primitives are to be drawn as
 * a triangle fan.
 *
 * The actual block index to use is determined by placing
 * bits into a byte as follows:
 * 0    COLOURE(0)
 * 1    COLOURE(1)
 * 2    COLOURE(2)
 * 3    COLOURE(3)
 * 4    COLOURC(0,1)
 * 5    COLOURC(1,2)
 * 6    COLOURC(2,3)
 * 7    COLOURC(3,4)
 *
 * As such, there are exactly 256 blocks (even though some
 * are nonsensical).
 */
//There are nine vertices, but we need to draw the second one
//twice to complete the square
#define INDEX_BLOCK_SZ 10
#define INDICES(x) \
  0, \
  1 + ((x)&(1<<4)? 1:0) + 0, \
  1 + ((x)&(1<<1)? 1:0) + 2, \
  1 + ((x)&(1<<5)? 1:0) + 4, \
  1 + ((x)&(1<<2)? 1:0) + 6, \
  1 + ((x)&(1<<6)? 1:0) + 8, \
  1 + ((x)&(1<<3)? 1:0) + 10, \
  1 + ((x)&(1<<7)? 1:0) + 12, \
  1 + ((x)&(1<<0)? 1:0) + 14, \
  1 + ((x)&(1<<4)? 1:0) + 0
#define INDICES2(x) INDICES(x), INDICES(x+1)
#define INDICES4(x) INDICES2(x), INDICES2(x+2)
#define INDICES8(x) INDICES4(x), INDICES4(x+4)
#define INDICES10(x) INDICES8(x), INDICES8(x+8)
#define INDICES20(x) INDICES10(x), INDICES10(x+16)
#define INDICES40(x) INDICES20(x), INDICES20(x+32)
#define INDICES80(x) INDICES40(x), INDICES40(x+64)
static const GLubyte backIxs[256*INDEX_BLOCK_SZ] = {
  INDICES80(0), INDICES80(128),
};
#undef INDICES80
#undef INDICES40
#undef INDICES20
#undef INDICES10
#undef INDICES8
#undef INDICES4
#undef INDICES2
#undef INDICES

#else /* defined(AB_OPENGL_14) */
namespace square_cell {
  GLuint initAccessoryLists() noth;
  //Display lists for all 16 possible accessory patterns,
  //accessed as a bitfield of the neighbours
  GLuint accessoryLists;
  bool accessoryListsInitialized=false;
};
using namespace square_cell;

struct circuit {
  bool neighbours[4];
  GLfloat vertices[6][2];
  int count;
};

circuit circuits[] = {
  { { true, false, false, false },
    { { +8*PIX, +5*PIX },
      { +5*PIX, +5*PIX },
      { +8*PIX, -5*PIX },
      { +5*PIX, -5*PIX },
      { +5*PIX, +5*PIX },
      { +5*PIX, -5*PIX },
    }, 6
  },
  { { false, false, true, false },
    { { -8*PIX, +5*PIX },
      { -5*PIX, +5*PIX },
      { -8*PIX, -5*PIX },
      { -5*PIX, -5*PIX },
      { -5*PIX, +5*PIX },
      { -5*PIX, -5*PIX },
    }, 6
  },
  { { true, false, true, false },
    { { +5*PIX, +5*PIX },
      { -5*PIX, +5*PIX },
      { +5*PIX, -5*PIX },
      { -5*PIX, -5*PIX },
    }, 4
  },
  { { false, false, false, true },
    { { +5*PIX, -8*PIX },
      { +5*PIX, -5*PIX },
      { -5*PIX, -8*PIX },
      { -5*PIX, -5*PIX },
      { +5*PIX, -5*PIX },
      { -5*PIX, -5*PIX },
    }, 6
  },
  { { false, true, false, false },
    { { +5*PIX, +8*PIX },
      { +5*PIX, +5*PIX },
      { -5*PIX, +8*PIX },
      { -5*PIX, +5*PIX },
      { +5*PIX, +5*PIX },
      { -5*PIX, +5*PIX },
    }, 6
  },
  { { false, true, false, true },
    { { +5*PIX, +5*PIX },
      { +5*PIX, -5*PIX },
      { -5*PIX, +5*PIX },
      { -5*PIX, -5*PIX },
    }, 4
  },
  { { true, false, false, true },
    { { +5*PIX, -8*PIX },
      { +8*PIX, -5*PIX },
      { -5*PIX, -5*PIX },
      { +5*PIX, +5*PIX },
    }, 4
  },
  { { true, true, false, false },
    { { +5*PIX, +8*PIX },
      { +8*PIX, +5*PIX },
      { -5*PIX, +5*PIX },
      { +5*PIX, -5*PIX },
    }, 4
  },
  { { false, true, true, false },
    { { -5*PIX, +8*PIX },
      { -8*PIX, +5*PIX },
      { +5*PIX, +5*PIX },
      { -5*PIX, -5*PIX },
    }, 4
  },
  { { false, false, true, true },
    { { -8*PIX, -5*PIX },
      { -5*PIX, -8*PIX },
      { -5*PIX, +5*PIX },
      { +5*PIX, -5*PIX },
    }, 4
  },
};

GLuint square_cell::initAccessoryLists() noth {
  if (headless) return 0;
  GLuint list=glGenLists(16);
  for (unsigned l=0; l<16; ++l) {
    bool neighbours[4] = { (bool)(l & 1), (bool)(l & 2),
                           (bool)(l & 4), (bool)(l & 8), };
    glNewList(list+l, GL_COMPILE);
    if (generalAlphaBlending)
      glColor4fv(semiblack);
    else
      glColor3fv(semiblack);
    glBegin(GL_LINES);
    for (unsigned int i=0; i<sizeof(circuits)/sizeof(circuit); ++i) {
      bool ok=true;
      for (int j=0; j<4; ++j)
        if (circuits[i].neighbours[j])
          ok &= neighbours[j];

      if (ok)
        for (int v=0; v<circuits[i].count; ++v)
          glVertex2fv(circuits[i].vertices[v]);
    }
    glEnd();
    glEndList();
  }
  return list;
}
#endif /* AB_OPENGL_14 */

SquareCell::SquareCell(Ship* ship)
: Cell(ship), hasCalculatedColourInfo(false)
{
  _intrinsicMass=50;
  isSquare=true;

#ifndef AB_OPENGL_14
  static bool hasVAO=false;
  if (!hasVAO && !headless) {
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
    glBindBuffer(GL_ARRAY_BUFFER, damVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(damVerts), damVerts, GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();

    hasVAO=true;
  }
#else
  if (!accessoryListsInitialized) accessoryLists=initAccessoryLists();
  accessoryListsInitialized=true;
#endif /* AB_OPENGL_14 */
}

unsigned SquareCell::numNeighbours() const noth {
  return 4;
}

float SquareCell::edgeD(int n) const noth {
  return STD_CELL_SZ/2;
}

int SquareCell::edgeT(int n) const noth {
  return 90*n;
}


void SquareCell::drawThis() noth {
  BEGINGP("SquareCell")
  drawSquareBase();
#ifdef AB_OPENGL_14
  drawAccessories();
  mUScale(STD_CELL_SZ/2);
#endif
  drawSystems();
  ENDGP
}

void SquareCell::drawSquareBase() noth {
#ifndef AB_OPENGL_14
  mUScale(STD_CELL_SZ/2);
  glBindVertexArray(backVao);
  glBindBuffer(GL_ARRAY_BUFFER, backVbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backIbo);
  shader::basic->activate(NULL);
  if (!hasCalculatedColourInfo) {
    COLOURC_PREP
    ixIx = COLOURE(0)
         | (COLOURE(1) << 1)
         | (COLOURE(2) << 2)
         | (COLOURE(3) << 3)
         | (COLOURC(0,1) << 4)
         | (COLOURC(1,2) << 5)
         | (COLOURC(2,3) << 6)
         | (COLOURC(3,0) << 7);
    hasCalculatedColourInfo=true;
  }
  glDrawElements(GL_TRIANGLE_FAN, INDEX_BLOCK_SZ, GL_UNSIGNED_BYTE,
                 reinterpret_cast<const GLvoid*>(ixIx*INDEX_BLOCK_SZ));

  glBindVertexArray(cxVao);
  glBindBuffer(GL_ARRAY_BUFFER, cxVbo);
  for (unsigned i=0; i<4; ++i) if (neighbours[i])
    glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);

  glBindVertexArray(cirVao);
  glBindBuffer(GL_ARRAY_BUFFER, cirVao);
  shader::quickU uni;
  uni.colour = Vec4(P_IX2F(P_BLACK),P_IX2F(P_BLACK),P_IX2F(P_BLACK),1);
  shader::quick->activate(&uni);

  glDrawArrays(GL_LINES, CIRCUIT_CENTRE_BEGIN, CIRCUIT_CENTRE_SZ);
  for (unsigned i=0; i<4; ++i) if (neighbours[i]) {
    glDrawArrays(GL_LINES, CIRCUIT_NEIGHBOUR_BEGIN+i*CIRCUIT_NEIGHBOUR_SZ, CIRCUIT_NEIGHBOUR_SZ);
    if (neighbours[(i+1)&3])
      glDrawArrays(GL_LINES, CIRCUIT_DUAL_NEIGHBOUR_BEGIN + i*CIRCUIT_DUAL_NEIGHBOUR_SZ,
                   CIRCUIT_DUAL_NEIGHBOUR_SZ);
  }
  //drawCircleOverlay() relies on the current state at this point
#else /* defined(AB_OPENGL_14) */
  glBegin(GL_QUADS);
  bool gradient=true;
  COLOURC_PREP
  if (!gradient) {
    glVertex2f(-STD_CELL_SZ/2, -STD_CELL_SZ/2);
    glVertex2f(+STD_CELL_SZ/2, -STD_CELL_SZ/2);
    glVertex2f(+STD_CELL_SZ/2, +STD_CELL_SZ/2);
    glVertex2f(-STD_CELL_SZ/2, +STD_CELL_SZ/2);
  } else {
    glVertex2f(0, 0);
    COLOURE(0);
    glVertex2f(+STD_CELL_SZ/2, 0);
    COLOURC(0,1);
    glVertex2f(+STD_CELL_SZ/2, +STD_CELL_SZ/2);
    COLOURE(1);
    glVertex2f(0, +STD_CELL_SZ/2);

    parent->glSetColour();
    glVertex2f(0,0);
    COLOURE(2);
    glVertex2f(-STD_CELL_SZ/2, 0);
    COLOURC(1,2);
    glVertex2f(-STD_CELL_SZ/2, +STD_CELL_SZ/2);
    COLOURE(1);
    glVertex2f(0, +STD_CELL_SZ/2);

    parent->glSetColour();
    glVertex2f(0,0);
    COLOURE(2);
    glVertex2f(-STD_CELL_SZ/2, 0);
    COLOURC(2,3);
    glVertex2f(-STD_CELL_SZ/2, -STD_CELL_SZ/2);
    COLOURE(3);
    glVertex2f(0, -STD_CELL_SZ/2);

    parent->glSetColour();
    glVertex2f(0,0);
    COLOURE(3);
    glVertex2f(0, -STD_CELL_SZ/2);
    COLOURC(3,0);
    glVertex2f(+STD_CELL_SZ/2, -STD_CELL_SZ/2);
    COLOURE(0);
    glVertex2f(+STD_CELL_SZ/2, 0);
  }

  glVertex2f(0, 0);
  COLOURE(0);
  glVertex2f(+STD_CELL_SZ/2, 0);
  COLOURC(0,1);
  glVertex2f(+STD_CELL_SZ/2, +STD_CELL_SZ/2);
  COLOURE(1);
  glVertex2f(0, +STD_CELL_SZ/2);

  parent->glSetColour();
  glVertex2f(0,0);
  COLOURE(2);
  glVertex2f(-STD_CELL_SZ/2, 0);
  COLOURC(1,2);
  glVertex2f(-STD_CELL_SZ/2, +STD_CELL_SZ/2);
  COLOURE(1);
  glVertex2f(0, +STD_CELL_SZ/2);

  parent->glSetColour();
  glVertex2f(0,0);
  COLOURE(2);
  glVertex2f(-STD_CELL_SZ/2, 0);
  COLOURC(2,3);
  glVertex2f(-STD_CELL_SZ/2, -STD_CELL_SZ/2);
  COLOURE(3);
  glVertex2f(0, -STD_CELL_SZ/2);

  parent->glSetColour();
  glVertex2f(0,0);
  COLOURE(3);
  glVertex2f(0, -STD_CELL_SZ/2);
  COLOURC(3,0);
  glVertex2f(+STD_CELL_SZ/2, -STD_CELL_SZ/2);
  COLOURE(0);
  glVertex2f(+STD_CELL_SZ/2, 0);
  glEnd();
#endif /* AB_OPENGL_14 */
}

void SquareCell::drawCircleOverlay() noth {
#ifndef AB_OPENGL_14
  //We rely on the state at the end of drawSquareBase()
  glDrawArrays(GL_LINES, CIRCUIT_CIRCLE_BEGIN, CIRCUIT_CIRCLE_SZ);
#endif /* AB_OPENGL_14 */
}

void SquareCell::drawSystems() noth {
  if (usage == CellBridge) {
    mUScale(2); //Cancel out STD_CELL_SZ/2
    mRot(-pi/2);
    mTrans(-0.5f,-0.5f);
    glBindTexture(GL_TEXTURE_2D, system_texture::squareBridge);
    square_graphic::bind();
    activateShipSystemShader();
    square_graphic::draw();
  } else {
    mUScale(2); //Cancel out the STD_CELL_SZ/2
    mRot(-pi/2);
    if (systems[0]) {
      mPush();
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
        mTrans(0.2f,0);
        //Rotate AFTER translation
        if (0 != systems[0]->getAutoRotate())
          mRot(systems[0]->getAutoRotate());
      }
      mTrans(-0.5f,-0.5f);
      systems[0]->draw();
      mPop();
    }

    if (systems[1]) {
      //No need to push, this is the last thing we do
      //Nudge to the left if nothing special
      if (systems[1]->positioning == ShipSystem::Centre
      ||  systems[1]->size == ShipSystem::Large) {
        //No movement
        if (0 != systems[1]->getAutoRotate())
          mRot(systems[1]->getAutoRotate());
      } else if (systems[1]->positioning == ShipSystem::Forward) {
        //Forward slightly
        if (0 != systems[1]->getAutoRotate())
          mRot(systems[1]->getAutoRotate());
        mTrans(0,0.2f);
      } else if (systems[1]->positioning == ShipSystem::Backward) {
        //Back slightly
        if (0 != systems[1]->getAutoRotate())
          mRot(systems[1]->getAutoRotate());
        mTrans(0,-0.2f);
      } else {
        mTrans(-0.2f,0);
        mRot(pi+systems[1]->getAutoRotate());
      }
      mTrans(-0.5f,-0.5f);
      systems[1]->draw();
    }
  }
}

void SquareCell::drawShapeThis(float,float,float,float) noth {
  BEGINGP("SquareCell::drawShapeThis")
  ENDGP
}

void SquareCell::drawDamageThis() noth {
#ifndef AB_OPENGL_14
  mUScale(STD_CELL_SZ/2);
  glBindVertexArray(damVao);
  glBindBuffer(GL_ARRAY_BUFFER,damVbo);
  shader::textureReplaceU uni;
  uni.colourMap=0;
  shader::textureReplace->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
  glBegin(GL_QUADS);
  glTexCoord2f(0,0);
  glVertex2f(-STD_CELL_SZ/2, -STD_CELL_SZ/2);
  glTexCoord2f(1,0);
  glVertex2f(+STD_CELL_SZ/2, -STD_CELL_SZ/2);
  glTexCoord2f(1,1);
  glVertex2f(+STD_CELL_SZ/2, +STD_CELL_SZ/2);
  glTexCoord2f(0,1);
  glVertex2f(-STD_CELL_SZ/2, +STD_CELL_SZ/2);
  glEnd();
#endif /* AB_OPENGL_14 */
}

#ifdef AB_OPENGL_14
void SquareCell::drawAccessories() noth {
  parent->glSetColour(0.5f);
  glBegin(GL_QUADS);
  if (neighbours[3]) {
    glVertex2f(+2*PIX, +2*PIX);
    glVertex2f(+2*PIX, -8*PIX);
    glVertex2f(-2*PIX, -8*PIX);
    glVertex2f(-2*PIX, +2*PIX);
  } if (neighbours[0]) {
    glVertex2f(-2*PIX, +2*PIX);
    glVertex2f(+8*PIX, +2*PIX);
    glVertex2f(+8*PIX, -2*PIX);
    glVertex2f(-2*PIX, -2*PIX);
  } if (neighbours[1]) {
    glVertex2f(+2*PIX, -2*PIX);
    glVertex2f(-2*PIX, -2*PIX);
    glVertex2f(-2*PIX, +8*PIX);
    glVertex2f(+2*PIX, +8*PIX);
  } if (neighbours[2]) {
    glVertex2f(+2*PIX, +2*PIX);
    glVertex2f(+2*PIX, -2*PIX);
    glVertex2f(-8*PIX, -2*PIX);
    glVertex2f(-8*PIX, +2*PIX);
  }
  glEnd();

  unsigned listoff = (neighbours[0]? 1 : 0)
                   | (neighbours[1]? 2 : 0)
                   | (neighbours[2]? 4 : 0)
                   | (neighbours[3]? 8 : 0);
  glCallList(accessoryLists+listoff);
}
#endif /* AB_OPENGL_14 */

#if 0
void SquareCell::drawSystems(bool highDetail) noth {
  if (usage==CellBridge) {
    glColor4fv(semiblack);
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(7*PIX, 0);
      glColor4fv(white);
      glVertex2f(-2*PIX, -8*PIX);
      glVertex2f(0, 0);
      glVertex2f(-2*PIX, +8*PIX);
    glEnd();
  } else {
    if (systems[0]) {
      glPushMatrix();
      switch (systems[0]->positioning) {
        case ShipSystem::Standard:
          glTranslatef(-4*PIX, -2*PIX, 0);
          break;
        case ShipSystem::Centre: break;
        case ShipSystem::Forward:
          //Rotate back to 0 so the coords are right
          glRotatef(-theta, 0, 0, 1);
          glTranslatef(+4*PIX, 0, 0);
          glRotatef(+theta, 0, 0, 1);
          break;
        case ShipSystem::Backward:
          glRotatef(-theta, 0, 0, 1);
          glTranslatef(-4*PIX, 0, 0);
          glRotatef(+theta, 0, 0, 1);
          break;
      }
      if (highDetail)
        systems[0]->draw();
      else
        systems[0]->drawLowDetail();
      glPopMatrix();
    } if (systems[1]) {
      glPushMatrix();
      switch (systems[1]->positioning) {
        case ShipSystem::Standard:
          glTranslatef(+4*PIX, +2*PIX, 0);
          break;
        case ShipSystem::Centre: break;
        case ShipSystem::Forward:
          glRotatef(-theta, 0, 0, 1);
          glTranslatef(+4*PIX, 0, 0);
          glRotatef(+theta, 0, 0, 1);
          break;
        case ShipSystem::Backward:
          glRotatef(-theta, 0, 0, 1);
          glTranslatef(-4*PIX, 0, 0);
          glRotatef(+theta, 0, 0, 1);
          break;
      }
      if (highDetail)
        systems[1]->draw();
      else
        systems[1]->drawLowDetail();
      glPopMatrix();
    }
  }
}
#endif /* 0 */

CollisionRectangle* SquareCell::getCollisionBounds() noth {
  pair<float,float> centre=Ship::cellCoord(parent, this);
  float ang = theta*pi/180 + parent->getRotation();
  float cang = cos(ang);
  float sang = sin(ang);
  float len = STD_CELL_SZ/2;
  #define VERT(i,h,v) \
    collisionRectangle.vertices[i]=make_pair( \
      centre.first + (h len)*cang - (v len)*sang, \
      centre.second + (v len)*cang + (h len)*sang)

  VERT(0,+,+);
  VERT(1,+,-);
  VERT(2,-,-);
  VERT(3,-,+);
  return &collisionRectangle;
  #undef VERT
}

float SquareCell::getIntrinsicDamage() const noth {
  return 10;
}
