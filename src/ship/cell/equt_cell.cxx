/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/equt_cell.hxx
 */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <GL/gl.h>

#include "equt_cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/globals.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/square.hxx"
#include "src/ship/ship_pallet.hxx"
#include "src/ship/sys/system_textures.hxx"

using namespace cell_colours;
using namespace std;

#define SQRT3 1.7320508f

#define TOSIDE SQRT3/6.0f
#define TOANGLE SQRT3/3.0f

#ifndef AB_OPENGL_14
static GLuint backVao, backVbo, backIbo,
              cxVao, cxVbo,
              cirVao, cirVbo,
              damVao, damVbo;

/* The main background follows the same schema
 * as for squares. See comments in square_cell.cxx
 * for a detailed explanation.
 * Indices are based in 6-bit blocks, however, as
 * follows:
 *   0  COLOURE(0)
 *   1  COLOURE(1)
 *   2  COLOURE(2)
 *   3  COLOURC(0,1)
 *   4  COLOURC(1,2)
 *   5  COLOURC(2,0)
 *
 * Non-centre vertices begin on the left corner
 * (we are oriented like |>, not /_\).
 */
#define DK P_IX2F(P_BODY_BEGIN)
#define LT P_IX2F(P_BODY_END)
#define VERT(x,y,l) { {{x,y,0,1}}, {{l,l,l,1}} }
#define VERTP(x,y) VERT(x,y,DK), VERT(x,y,LT)
static const shader::VertCol backVerts[] = {
  VERT(0,0,LT),
  VERTP(2*TOANGLE*cos(0*pi/3*2), 2*TOANGLE*sin(0*pi/3*2)),
  VERTP(2*TOANGLE*(cos(0*pi/3*2)+cos(1*pi/3*2))/2, 2*TOANGLE*sin(1*pi/3*2)/2),
  VERTP(2*TOANGLE*cos(1*pi/3*2), 2*TOANGLE*sin(1*pi/3*2)),
  VERTP(2*TOANGLE*(cos(1*pi/3*2)+cos(2*pi/3*2))/2, 2*TOANGLE*(sin(1*pi/3*2)+sin(2*pi/3*2))/2),
  VERTP(2*TOANGLE*cos(2*pi/3*2), 2*TOANGLE*sin(2*pi/3*2)),
  VERTP(2*TOANGLE*(cos(2*pi/3*2)+cos(0*pi/3*2))/2, 2*TOANGLE*sin(2*pi/3*2)/2),
};
#undef VERTP
#undef LT
#undef DK
#undef VERT

static const shader::VertTexc damVerts[3] = {
  { {{2*TOANGLE*cos(0*pi/3*2),2*TOANGLE*sin(0*pi/3*2),0,1}}, {{1,0.5f}} },
  { {{2*TOANGLE*cos(1*pi/3*2),2*TOANGLE*sin(1*pi/3*2),0,1}}, {{0,0}} },
  { {{2*TOANGLE*cos(2*pi/3*2),2*TOANGLE*sin(2*pi/3*2),0,1}}, {{0,1}} },
};


/* I couldn't figure out any good way to do the calculations below,
 * so I'm copying the aweful equations I used under the old
 * OpenGL API. Speed obviously doesn't matter here, since it is run
 * at load time (or possibly compile time) and not every frame.
 */
/*Relative to the side itself, as if the side were vertical right
//#define _PIXXY(side,x,y) ((x)*TOSIDE*cos((side)*pi/3*2 + pi/3) + (y)*0.5f*sin((side)*pi/3*2 + pi/3)) * STD_CELL_SZ/8, \
//                         ((y)*0.5f*cos((side)*pi/3*2 + pi/3) + (x)*TOSIDE*sin((side)*pi/3*2 + pi/3)) * STD_CELL_SZ/8
//I derived this by flipping the (x,y) into polar coörds, rotating, and flipping back
//There MUST be a better way to do this (the above doesn't work...), but, since this is only
//executed once for most things, I figure I can get away with this horrible formula
 */
#define __PIXXY(side,x,y) (sqrt((x)*(x) + (y)*(y))*cos(atan2(y,x) + (side)*pi/3*2+pi/3)), \
                          (sqrt((x)*(x) + (y)*(y))*sin(atan2(y,x) + (side)*pi/3*2+pi/3))
#define _PIXXY(side,x,y) __PIXXY(side, x*TOSIDE*2, y)
#define PIXXY(side,x,y) { {{_PIXXY(side,x,y),0,1}},\
  {{P_IX2F((P_BODY_END+P_BODY_BEGIN)/2),\
    P_IX2F((P_BODY_END+P_BODY_BEGIN)/2),\
    P_IX2F((P_BODY_END+P_BODY_BEGIN)/2),1}} }
static const shader::VertCol cxnVerts[12] = {
  PIXXY(0, +0,-0.2f), PIXXY(0, +1,-0.2f), PIXXY(0,+0,+0.2f), PIXXY(0,+1,+0.2f),
  PIXXY(1, +0,-0.2f), PIXXY(1, +1,-0.2f), PIXXY(1,+0,+0.2f), PIXXY(1,+1,+0.2f),
  PIXXY(2, +0,-0.2f), PIXXY(2, +1,-0.2f), PIXXY(2,+0,+0.2f), PIXXY(2,+1,+0.2f),
};

/* Circuits also work the same as in square_cell, including the same
 * boundaries. One difference is the lack of DUAL_NEIGHBOURS, since
 * the diagonal line would be the border of the cell.
 */
#undef PIXXY
#define PIXXY(side,x,y) { {{_PIXXY(side,x,y)}} }
static const shader::Vert2 cirVerts[] = {
  //BEGIN: CENTRE
  PIXXY(0,+0.4f,+0.4f), PIXXY(0,+0.4f,-0.4f), //0
  PIXXY(1,+0.4f,+0.4f), PIXXY(1,+0.4f,-0.4f), //2
  PIXXY(2,+0.4f,+0.4f), PIXXY(2,+0.4f,-0.4f), //4
  //END: CENTRE
  //BEGIN: NEIGHBOURS
  PIXXY(0,+1.0f,+0.4f), PIXXY(0,+0.4f,+0.4f), //6, 0
  PIXXY(0,+1.0f,-0.4f), PIXXY(0,+0.4f,-0.4f), //8, 0
  PIXXY(1,+1.0f,+0.4f), PIXXY(1,+0.4f,+0.4f), //10, 1
  PIXXY(1,+1.0f,-0.4f), PIXXY(1,+0.4f,-0.4f), //12, 1
  PIXXY(2,+1.0f,+0.4f), PIXXY(2,+0.4f,+0.4f), //14, 2
  PIXXY(2,+1.0f,-0.4f), PIXXY(2,+0.4f,-0.4f), //16, 2
  //END: NEIGHBOURS
};
#define CIRCUIT_CENTRE_BEGIN 0
#define CIRCUIT_CENTRE_SZ 6
#define CIRCUIT_NEIGHBOUR_BEGIN 6
#define CIRCUIT_NEIGHBOUR_SZ 4

#define INDEX_BLOCK_SZ 8
#define INDICES(x) \
  0,\
  1 + ((x)&(1<<5)?1:0) + 0,\
  1 + ((x)&(1<<0)?1:0) + 2,\
  1 + ((x)&(1<<3)?1:0) + 4,\
  1 + ((x)&(1<<1)?1:0) + 6,\
  1 + ((x)&(1<<4)?1:0) + 8,\
  1 + ((x)&(1<<2)?1:0) + 10,\
  1 + ((x)&(1<<5)?1:0) + 0
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
struct equt_circuit {
  bool neighbours[3];
  GLfloat vertices[6][2];
  int count;
};

/*Relative to the side itself, as if the side were vertical right
//#define _PIXXY(side,x,y) ((x)*TOSIDE*cos((side)*pi/3*2 + pi/3) + (y)*0.5f*sin((side)*pi/3*2 + pi/3)) * STD_CELL_SZ/8, \
//                         ((y)*0.5f*cos((side)*pi/3*2 + pi/3) + (x)*TOSIDE*sin((side)*pi/3*2 + pi/3)) * STD_CELL_SZ/8
//I derived this by flipping the (x,y) into polar coörds, rotating, and flipping back
//There MUST be a better way to do this (the above doesn't work...), but, since this is only
//executed once for most things, I figure I can get away with this horrible formula
 */
#define __PIXXY(side,x,y) (sqrt((x)*(x) + (y)*(y))*cos(atan2(y,x) + (side)*pi/3*2+pi/3)), \
                          (sqrt((x)*(x) + (y)*(y))*sin(atan2(y,x) + (side)*pi/3*2+pi/3))
#define _PIXXY(side,x,y) __PIXXY(side, x*TOSIDE*STD_CELL_SZ/8, y/2.0f*STD_CELL_SZ/8)
#define PIXXY(side,x,y) { _PIXXY(side,x,y) }
equt_circuit equt_circuits[] = {
  { { false, true, false },
    { PIXXY(1, +8, +5),
      PIXXY(1, +5, +5),
      PIXXY(1, +8, -5),
      PIXXY(1, +5, -5),
    }, 4
  },
  { { true, false, false },
    { PIXXY(0, +8, +5),
      PIXXY(0, +5, +5),
      PIXXY(0, +8, -5),
      PIXXY(0, +5, -5),
    }, 4
  },
  { { false, false, true },
    { PIXXY(2, +8, +5),
      PIXXY(2, +5, +5),
      PIXXY(2, +8, -5),
      PIXXY(2, +5, -5),
    }, 4
  },
  //ALWAYS draw the central triangle
  { { false, false, false},
    { PIXXY(1, +5, +5),
      PIXXY(1, +5, -5),
      PIXXY(0, +5, +5),
      PIXXY(0, +5, -5),
      PIXXY(2, +5, +5),
      PIXXY(2, +5, -5),
    }, 6
  },
};

namespace equt_verts {
  float vertices[3][2] = {
    //Rendundancy (like cos(0*pi/3)) to show pattern
    { TOANGLE*STD_CELL_SZ*cos(0*pi/3*2), TOANGLE*STD_CELL_SZ*sin(0*pi/3*2) },
    { TOANGLE*STD_CELL_SZ*cos(1*pi/3*2), TOANGLE*STD_CELL_SZ*sin(1*pi/3*2) },
    { TOANGLE*STD_CELL_SZ*cos(2*pi/3*2), TOANGLE*STD_CELL_SZ*sin(2*pi/3*2) },
  };

  //The other "vertices" for gradients; the midpoints of the sides
  #define AVG(a,b) { (vertices[a][0]+vertices[b][0])/2, (vertices[a][1]+vertices[b][1])/2 }
  float altverts[3][2] = {
    AVG(0,1), AVG(1,2), AVG(2,0)
  };
  #undef AVG
};
using namespace equt_verts;
#endif /* AB_OPENGL_14 */

EquTCell::EquTCell(Ship* parent)
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
    glBindBuffer(GL_ARRAY_BUFFER,backVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,backIbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(backVerts),backVerts,GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(backIxs),backIxs,GL_STATIC_DRAW);
    shader::basic->setupVBO();

    cxVao=newVAO();
    glBindVertexArray(cxVao);
    cxVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER,cxVbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cxnVerts),cxnVerts,GL_STATIC_DRAW);
    shader::basic->setupVBO();

    cirVao=newVAO();
    glBindVertexArray(cirVao);
    cirVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER,cirVbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cirVerts),cirVerts,GL_STATIC_DRAW);
    shader::quick->setupVBO();

    damVao=newVAO();
    glBindVertexArray(damVao);
    damVbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER,damVbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(damVerts),damVerts,GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();

    hasVao=true;
  }
#endif /* AB_OPENGL_14 */
}

unsigned EquTCell::numNeighbours() const noth {
  return 3;
}

float EquTCell::edgeD(int n) const noth {
  return TOSIDE*STD_CELL_SZ;
}

int EquTCell::edgeT(int n) const noth {
  return 120*n + 60;
}

static const int equt_cell_dts[3]={-30, 0, 30};
int EquTCell::edgeDT(int n) const noth {
  return equt_cell_dts[n];
}


void EquTCell::drawThis() noth {
  BEGINGP("EquTCell")
#ifndef AB_OPENGL_14
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

  mUScale(STD_CELL_SZ/2);
  glBindVertexArray(backVao);
  glBindBuffer(GL_ARRAY_BUFFER, backVbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backIbo);
  shader::basic->activate(NULL);
  glDrawElements(GL_TRIANGLE_FAN, INDEX_BLOCK_SZ, GL_UNSIGNED_BYTE,
                 reinterpret_cast<const GLvoid*>(ixIx*INDEX_BLOCK_SZ));

  glBindVertexArray(cxVao);
  glBindBuffer(GL_ARRAY_BUFFER,cxVbo);
  for (unsigned i=0; i<3; ++i)
    if (neighbours[i])
      glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);

  glBindVertexArray(cirVao);
  glBindBuffer(GL_ARRAY_BUFFER, cirVbo);
  shader::Colour c = {{{P_IX2F(P_BLACK),P_IX2F(P_BLACK),P_IX2F(P_BLACK),1}}};
  shader::quick->activate(&c);
  glDrawArrays(GL_LINES, CIRCUIT_CENTRE_BEGIN, CIRCUIT_CENTRE_SZ);
  for (unsigned i=0; i<3; ++i)
    if (neighbours[i])
      glDrawArrays(GL_LINES, CIRCUIT_NEIGHBOUR_BEGIN+i*CIRCUIT_NEIGHBOUR_SZ, CIRCUIT_NEIGHBOUR_SZ);

#else /* defined(AB_OPENGL_14) */
  glBegin(GL_TRIANGLES);
  bool gradient=true;
  if (!gradient) {
    for (int i=0; i<3; ++i)
      glVertex2fv(vertices[i]);
  } else {
    COLOURC_PREP
    for (int i=0; i<3; ++i) {
      parent->glSetColour();
      glVertex2f(0,0);
      COLOURC((i==0? 2:i-1),i);
      glVertex2fv(vertices[i]);
      COLOURE(i);
      glVertex2fv(altverts[i]);

      parent->glSetColour();
      glVertex2f(0,0);
      COLOURE(i);
      glVertex2fv(altverts[i]);
      COLOURC(i,(i+1)%3);
      glVertex2fv(vertices[(i+1)%3]);
    }
  }
  glEnd();
  glBindTexture(GL_TEXTURE_2D, 0);

  parent->glSetColour(0.5f);
  glBegin(GL_QUADS);
  for (int i=0; i<3; ++i)
    if (neighbours[i]) {
      glVertex2f(_PIXXY(i, +8, +2));
      glVertex2f(_PIXXY(i, +8, -2));
      glVertex2f(_PIXXY(i, +0, -2));
      glVertex2f(_PIXXY(i, +0, +2));
    }
  glEnd();

  if (generalAlphaBlending)
    glColor4fv(semiblack);
  else
    glColor3fv(semiblack);
  glBegin(GL_LINES);
  for (unsigned int i=0; i<sizeof(equt_circuits)/sizeof(equt_circuit); ++i) {
    bool ok=true;
    for (int n=0; n<3; ++n)
      if (equt_circuits[i].neighbours[n])
        ok &= (this->neighbours[n] != NULL);

    if (ok)
      for (int v=0; v<equt_circuits[i].count; ++v)
        glVertex2fv(equt_circuits[i].vertices[v]);
  }
  glEnd();
  mUScale(STD_CELL_SZ/2);
#endif /* AB_OPENGL_14 */

  if (usage == CellBridge) {
    mUScale(2); //Cancel out STD_CELL_SZ/2
    mRot(-pi/2);
    mTrans(-0.5f,-0.5f);
    glBindTexture(GL_TEXTURE_2D, system_texture::equtBridge);
    square_graphic::bind();
    activateShipSystemShader();
    square_graphic::draw();
  } else {
    if (systems[0]) {
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
  }

  ENDGP
}

void EquTCell::drawShapeThis(float,float,float,float) noth {
  BEGINGP("EqutCell::drawShapeThis")
  ENDGP
}

void EquTCell::drawDamageThis() noth {
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

CollisionRectangle* EquTCell::getCollisionBounds() noth {
  //We're actually going to make a quadrilatteral with
  //three collinear vertices
  float vtheta=theta*pi/180 + parent->getRotation();
  pair<float,float> coords=Ship::cellCoord(parent, this);
  float cx=coords.first,
        cy=coords.second;
  #define VERT(i,th) collisionRectangle.vertices[i].first=TOANGLE*STD_CELL_SZ*cos(vtheta+(th)) + cx; \
                     collisionRectangle.vertices[i].second=TOANGLE*STD_CELL_SZ*sin(vtheta+(th)) + cy

  VERT(0, 0*pi/3*2);
  VERT(1, 1*pi/3*2);
  VERT(2, 2*pi/3*2);
  //The forth is just halfway between points 2 and 0
  collisionRectangle.vertices[3].first =TOSIDE*STD_CELL_SZ*cos(vtheta-pi/3) + cx;
  collisionRectangle.vertices[3].second=TOSIDE*STD_CELL_SZ*sin(vtheta-pi/3) + cy;
  #undef VERT
  return &collisionRectangle;
}

float EquTCell::getIntrinsicDamage() const noth {
  return 11;
}
