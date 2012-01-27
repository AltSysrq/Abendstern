/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/auxobj/cell_fragment.hxx
 */

#include <GL/gl.h>
#include <cstdlib>
#include <cmath>
#include <utility>
#include <iostream>

#include "cell_fragment.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/blast.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/globals.hxx"

using namespace std;

#define TOTAL_LIFE 5000
#define TEXSZ 32

#define MIN_FRAGS 3
#define MAX_FRAGS 6

CellFragment::CellFragment(GameField* field, Cell* par, float x, float y, float vx, float vy,
                           const shader::quickU& col)
: GameObject(field, x, y, vx + 0.0005f*rand()/(float)RAND_MAX, vy + 0.0005f*rand()/(float)RAND_MAX),
  theta(par->parent->getRotation() + par->getT()*pi/180),
  vtheta(0.003f*rand()/(float)RAND_MAX),
  mass(par->getPhysics().mass * getAreaAndSetVertices() / STD_CELL_SZ/STD_CELL_SZ),
  vao(newVAO()), vbo(initVBO()),
  lifeLeft(TOTAL_LIFE), colour(col)
{
  decorative=true;
  okToDecorate();
  includeInCollisionDetection=false;
}

CellFragment::~CellFragment() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

GLuint CellFragment::initVBO() const noth {
  glBindVertexArray(vao);
  GLuint vbo=newVBO();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  shader::quick->setupVBO();
  return vbo;
}

float CellFragment::getAreaAndSetVertices() const noth {
  for (int i=0; i<3; ++i) for (int j=0; j<2; ++j)
    vertices[i].vertex[j]=rand()/(float)RAND_MAX*2*STD_CELL_SZ - STD_CELL_SZ;

  /* Area = 0.5*abs(x1*y2 - x2*y1)
   * where x1 = v2.x-v1.x,
   *       x2 = v3.x-v1.x,
   *       y1 = v2.y-v1.y,
   *       y2 = v3.y-v1.y
   * See also:
   * http://en.wikipedia.org/wiki/Area_of_triangle#Computing_the_area_of_a_triangle
   */
  return 0.5f*fabs((vertices[1].vertex[0]-vertices[0].vertex[0])*(vertices[2].vertex[1]-vertices[0].vertex[1]) -
                   (vertices[2].vertex[0]-vertices[0].vertex[0])*(vertices[1].vertex[1]-vertices[0].vertex[1]));
}

void CellFragment::spawn(Cell* parent, Blast* blast) noth {
  if (headless || !highQuality) return;

  pair<float,float> coord=Ship::cellCoord(parent->parent, parent);
  if (!EXPCLOSE(coord.first, coord.second)) return;
  pair<float,float> vel=parent->parent->getCellVelocity(parent);

  int cnt = (rand()%(MAX_FRAGS-MIN_FRAGS))+MIN_FRAGS;
  float f = 0.6f * (1 - parent->getCurrDamage()/parent->getMaxDamage());
  const shader::quickU colour = {{{f*parent->parent->getColourR(), f*parent->parent->getColourG(),
                                   f*parent->parent->getColourB(), 1}}};
  while (cnt--) {
    CellFragment* f=new CellFragment(parent->parent->getField(), parent,
                                     coord.first, coord.second, vel.first, vel.second, colour);
    f->collideWith(blast);
    parent->parent->getField()->addBegin(f);
  }
}

bool CellFragment::update(float et) noth {
  x += vx*et;
  y += vy*et;
  theta += vtheta*et;
  colour.colour[3] -= et/TOTAL_LIFE;
  //If spinning too quickly, we might as well jitter randomly
  if (theta < -10*pi || theta > 10*pi || theta != theta) {
    theta = 2*pi*(rand()/(float)RAND_MAX);
  } else {
    while (theta>2*pi) theta-=2*pi;
    while (theta<0)    theta+=2*pi;
  }

  lifeLeft-=et;

  return lifeLeft>0;
}

void CellFragment::draw() noth {
  BEGINGP("CellFragment")
  mPush();
  mTrans(x, y);
  mRot(theta);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::quick->activate(&colour);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  mPop();
  ENDGP
}

bool CellFragment::collideWith(GameObject* other) noth {
  if (other==this) return false;

  //Assume a blast
  Blast* blast=(Blast*)other;

  //Average the vertices together to find out where we actually appear
  float vertXOff = (vertices[0].vertex[0] + vertices[1].vertex[0] + vertices[2].vertex[0])/3.0f,
        vertYOff = (vertices[1].vertex[0] + vertices[1].vertex[1] + vertices[2].vertex[1])/3.0f;
  float virtX = x + STD_CELL_SZ*(vertXOff*cos(theta) - vertYOff*sin(theta));
  float virtY = y + STD_CELL_SZ*(vertYOff*cos(theta) + vertXOff*sin(theta));
  float dx = blast->getX() - virtX,
        dy = blast->getY() - virtY;
  float dist = sqrt(dx*dx + dy*dy);
  float cosine = dx/dist, sine=dy/dist;

  //Divide by 2 since that's how we define the movement effect of a Blast
  float strength = blast->getStrength(dist/2) / mass / 2500;

  vx -= strength*cosine, vy -= strength*sine;
  vtheta += strength * (rand()/(float)RAND_MAX - 0.5f);

  return true;
}
