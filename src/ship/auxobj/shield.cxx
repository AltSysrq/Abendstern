/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/auxobj/shield.hxx
 */

#include <GL/gl.h>
#include <cmath>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <vector>

#include "shield.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/blast.hxx"
#include "src/sim/collision.hxx"
#include "src/fasttrig.hxx"
#include "src/globals.hxx"
#include "src/audio/ship_mixer.hxx"
#include "src/audio/ship_effects.hxx"

#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/square.hxx"
#include "src/graphics/gl32emu.hxx"

using namespace std;

#define STRENGTH_MUL 5
#define MAX_IMPACT_VOL 3.0f
#define IMPACT_VOL_MUL 3.0f

void Shield::initBases() noth {
  for (unsigned i=0; i<lenof(baseRects); ++i) {
    for (unsigned v=0; v<4; ++v) {
      float angle = pi/2.0f*i/(float)lenof(baseRects) + pi/2.0f*(float)v;
      baseRects[i].vertices[v].first  = STD_CELL_SZ*radius*cos(angle);
      baseRects[i].vertices[v].second = STD_CELL_SZ*radius*sin(angle);
    }
  }
}

/* This function, for some reason, takes an enormous
 * amount of CPU time.
 * Before HOT: 11.5%
 * After HOT:  10.8%
 * Lazy *off:   9.5%
 * radiusInv/Sq 8.5%
 * inln hasPow  8.81% (within random error of 8.5%)
 * switch logic 4.58%
 *
 * The optimized version runs in 39.8% the time as the original.
 */
void Shield::update(float et) noth {
  needUpdateXYOffs=true;

  //It's OK to be negative
  alpha -= et*0.001f;

  switch (healState) {
    case Full:
    case Dead: break;
    case Stabilize:
      //Regain stability at 200%/r/sec
      stability += et*0.002f*radiusInv;
      if (stability >= 1.0f) {
        stability=1.0f;
        healState=Heal;
      }
      break;
    case Heal:
      //Regain strength at 2.5dmg/sec
      strength += et*0.0025f;
      if (strength >= maxStrength) {
        strength=maxStrength;
        healState=Full;
      }
      break;
  }

  //Update collision rectangles if we must
  needUpdateRects=true;
}

void Shield::updateDist() noth {
  dist = sqrt(parent->getX()*parent->getX() + parent->getY()*parent->getY());
  if (!parent->parent->hasPower()) healState=Dead;
}

#ifndef AB_OPENGL_14
#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE shader::Colour
DELAY_SHADER(shield)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord),
  NULL,
  true,
  UNIFORM(colour),
  NULL
END_DELAY_SHADER(static shieldShader);
#endif /* AB_OPENGL_14 */

void Shield::draw() noth {
  if (alpha<=0.0f || !parent) return;

  BEGINGP("Shield")
#ifndef AB_OPENGL_14
  if (needUpdateXYOffs) updateXYOffs();
  Ship* par=parent->parent;
  float pr=par->getColourR(), pg=par->getColourG(), pb=par->getColourB();
  float red=(pg+pb)/2, grn=(pr+pb)/2, blu=(pr+pg)/2;
  float cx = par->getX()+xoff, cy = par->getY()+yoff;
  UNIFORM_TYPE uni = {{{red,grn,blu,alpha}}};
  mPush();
  mTrans(cx,cy);
  mUScale(2*radius*STD_CELL_SZ);
  mTrans(-0.5f,-0.5f);
  square_graphic::bind();
  shieldShader->activate(&uni);
  square_graphic::draw();
  mPop();
#else /* defined(AB_OPENGL_14) */
  #define NUM_VERTICES 10
  if (needUpdateXYOffs) updateXYOffs();

  Ship* par = parent->parent;
  glBegin(GL_TRIANGLE_FAN);
  float pr=par->getColourR(), pg=par->getColourG(), pb=par->getColourB();
  float red=(pg+pb)/2, grn=(pr+pb)/2, blu=(pr+pg)/2;
  float cx = par->getX()+xoff, cy = par->getY()+yoff;
  glColor4f(red, grn, blu, 0);
  glVertex2f(cx, cy);
  glColor4f(red, grn, blu, alpha);
  for (unsigned i=0; i<TRIGTABLE_SZ; i+=TRIGTABLE_SZ/NUM_VERTICES)
    glVertex2f(cx + STD_CELL_SZ*radius*FCOS(i), cy + STD_CELL_SZ*radius*FSIN(i));
  //Close circle
  glVertex2f(cx + STD_CELL_SZ*radius, cy);
  glEnd();

#endif /* AB_OPENGL_14 */
  ENDGP
}

/* Represent the Shield on the HUD by hollowing the shield
 * when strength is reduced, and reducing the alpha in
 * accordance with the stability.
 */
void Shield::drawForHUD(float r, float g, float b, float a) noth {
  return; //Not yet implemented
}

float Shield::getRadius() const noth {
  if (!parent) {
    //float a=0; return a/a;
    return NAN;
  }
  return dist + this->radius*STD_CELL_SZ;
}

void Shield::addCollisionBounds(vector<CollisionRectangle*>& vec) noth {
  if (strength == 0) return;

  if (needUpdateRects) {
    if (needUpdateXYOffs) updateXYOffs();

    float cx = parent->parent->getX()+xoff, cy = parent->parent->getY()+yoff;
    for (unsigned i=0; i<lenof(collisionRects); ++i) {
      for (unsigned v=0; v<4; ++v) {
        collisionRects[i].vertices[v].first  = cx + baseRects[i].vertices[v].first;
        collisionRects[i].vertices[v].second = cy + baseRects[i].vertices[v].second;
      }
    }
    needUpdateRects=false;
  }

  vec.insert(vec.end(),
             collisionRectPtrs, collisionRectPtrs+lenof(collisionRectPtrs));
}

void Shield::updateXYOffs() noth {
  Ship* par=parent->parent;
  pair<float,float> coord=Ship::cellCoord(par, parent);
  xoff=coord.first-par->getX();
  yoff=coord.second-par->getY();
  needUpdateXYOffs=false;
}

bool Shield::collideWith(AObject* that) noth {
  if (!parent || that==this) return false;
  if (typeid(*that) == typeid(Blast)) {
    if (needUpdateXYOffs) updateXYOffs();
    Blast* blast=(Blast*)that;
    if (!blast->causesDamage() || strength==0) return true;

    float cx=parent->parent->getX()+xoff, cy=parent->parent->getY()+yoff;
    float dx=blast->getX()-cx, dy=blast->getY()-cy;
    float dist=sqrt(dx*dx + dy*dy);
    if (dist > radius*STD_CELL_SZ+STD_CELL_SZ/2) dist-=radius*STD_CELL_SZ+STD_CELL_SZ/2;
    else dist=0;
    float str = blast->getStrength(dist);
    if (str == 0) return true;

    if (parent->parent->soundEffectsEnabled())
      audio::shipDynamicEvent(audio::shieldImpactSound(), audio::shieldImpactLength(), parent,
                              min(MAX_IMPACT_VOL, IMPACT_VOL_MUL*str/STRENGTH_MUL/strength));

    stability=0;
    alpha=1;
    strength -= str/STRENGTH_MUL * parent->parent->damageMultiplier;
    if (strength < 0) {
      strength=0;
    }

    if (healState != Dead) healState=Stabilize;

    parent->parent->applyCollision(cx, cy, str, blast->getX(), blast->getY());
  }
  return true;
}
