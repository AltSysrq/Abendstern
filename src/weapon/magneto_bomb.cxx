/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/magneto_bomb.hxx
 */

#include <cmath>
#include <vector>
#include <cstdlib>
#include <typeinfo>
#include <iostream>

#include <GL/gl.h>

#include "magneto_bomb.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"
#include "src/sim/blast.hxx"
#include "src/sim/collision.hxx"
#include "src/background/explosion.hxx"
#include "src/ship/ship.hxx"
//For STD_CELL_SZ
#include "src/ship/cell/cell.hxx"
#include "src/ship/auxobj/shield.hxx"

#include "src/graphics/square.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#ifdef AB_OPENGL_14
#include "src/graphics/gl32emu.hxx"
#endif

using namespace std;

#define ROTATION_SPEED (2*pi/1000.0f)
//#define CORE_RADIUS (sqrt(power)/500.0f)
#define CORE_RADIUS coreRadius
#define SPIKE_MAX_RADIUS (CORE_RADIUS*5)
#define SPIKE_MOVE_SPEED ((SPIKE_MAX_RADIUS-CORE_RADIUS)/100.0f)
#define NUM_COLLISION_RECTS 3
#define ATTRACTION ((float)(0.5/1000.0/1000.0/1000.0*STD_CELL_SZ))
#define GUIDANCE_ACTIVATE_TIME 250.0f
#define NUM_MAGNETO_GLARES 6

namespace magneto_bomb {
  CollisionRectangle* generateBases() noth {
    CollisionRectangle *rects = new CollisionRectangle[NUM_COLLISION_RECTS];
    for (unsigned int i=0; i<NUM_COLLISION_RECTS; ++i) {
      float baseAngle=i*pi*2/NUM_COLLISION_RECTS;
      for (int v=0; v<4; ++v) {
        rects[i].vertices[v].first=cos(baseAngle + v*pi/2);
        rects[i].vertices[v].second=sin(baseAngle + v*pi/2);
      }
    }
    return rects;
  }
  CollisionRectangle *baseCollisionRects = generateBases();
};

using magneto_bomb::baseCollisionRects;

#ifndef AB_OPENGL_14
struct MBUniform {
  vec4 modColour;
  float rotation;
};

#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE MBUniform
DELAY_SHADER(magnetoBomb)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord),
  NULL,
  true,
  UNIFORM(modColour),
  UNIFLOAT(rotation),
  NULL
END_DELAY_SHADER(static mbshader);
#endif /* AB_OPENGL_14 */

MagnetoBomb::MagnetoBomb(GameField* field, float x, float y, float vx,
                         float vy, float _power, Ship* _parent,
                         float subMult, float _r, float _g, float _b)
: GameObject(field, x, y, vx, vy),
  power(_power*subMult), coreRadius(sqrt(_power*subMult)/500.0f),
  halflife(1000 + rand()/(float)RAND_MAX*100000.0f/_power),
  armTime(100), inParentsShields(true), hitParentsShields(true),
  rotation(0), parent(_parent),
  r(_r), g(_g), b(_b), exploded(false),
  ax(0), ay(0), physFrameCount(0),
  timeAlive(0), blame(_parent? _parent->blame : 0xFFFFFF)
{
  for (unsigned i=0; i<lenof(collisionRects); ++i) {
    collisionRects[i].radius=2*CORE_RADIUS;
    collisionBounds.push_back(&collisionRects[i]);
  }

  classification = GameObject::HeavyWeapon;
  isExportable=true;
}

bool MagnetoBomb::update(float time) noth {
  //Only update velocities on physical frame boundaries with local bombs
  if (currentVFrameLast && 5 == ++physFrameCount) {
    physFrameCount=0;
    unsigned int size=field->size();
    for (unsigned int i=0; i<size; ++i) {
      GameObject* obj=(*field)[i];
      //Skip objects farther than one screen away
      if (obj->getX()<x-1 || obj->getX()>x+1 ||
          obj->getY()<y-1 || obj->getY()>y+1) continue;
      if (typeid(*obj)==typeid(Ship)) {
        Ship* ship=(Ship*)obj;
        float dx=ship->getX()-x, dy=ship->getY()-y;
        float dist=sqrt(dx*dx + dy*dy);
        if (dist < 0.001f) continue;
        float cosine=dx/dist, sine=dy/dist;

        float guidanceActivated = (timeAlive < GUIDANCE_ACTIVATE_TIME?
                                   timeAlive/GUIDANCE_ACTIVATE_TIME : 1);
        ax= (float)(cosine*ATTRACTION*ship->getMass()/dist * guidanceActivated);
        ay= (float)(sine  *ATTRACTION*ship->getMass()/dist * guidanceActivated);
      }
    }
  }

  vx += ax*time;
  vy += ay*time;

  return coreUpdate(time);
}

bool MagnetoBomb::coreUpdate(float time) noth {
  timeAlive+=time;

  if (armTime <= 0) {
    inParentsShields=hitParentsShields;
    hitParentsShields=false;
  }
  x+=vx*time;
  y+=vy*time;
  rotation += ROTATION_SPEED*time;

  if (isRemote) {
    REMOTE_XYCK;
  }

  if (armTime>0) armTime-=time;
  else if ((halflife-=time) < 0 && !isRemote) {
    explode();
    return false;
  }

  //Update collision bounds
  float radius=CORE_RADIUS;
  for (unsigned int i=0; i<NUM_COLLISION_RECTS; ++i) {
    for (int v=0; v<4; ++v) {
      collisionRects[i].vertices[v].first =
        x + radius*baseCollisionRects[i].vertices[v].first;
      collisionRects[i].vertices[v].second =
      y + radius*baseCollisionRects[i].vertices[v].second;
    }
  }
  return true;
}

void MagnetoBomb::draw() noth {
#ifndef AB_OPENGL_14
  BEGINGP("MagnetoBomb")
  float radius=SPIKE_MAX_RADIUS;
  MBUniform uni;
  uni.modColour = Vec4(r,g,b,1);
  uni.rotation=rotation;
  square_graphic::bind();
  mPush();
  mTrans(x,y);
  mUScale(radius*(timeAlive*2 < GUIDANCE_ACTIVATE_TIME?
                  timeAlive*2 / GUIDANCE_ACTIVATE_TIME : 1));
  mTrans(-0.5f,-0.5f);
  mbshader->activate(&uni);
  square_graphic::draw();
  mPop();
  ENDGP
#else /* defined(AB_OPENGL_14) */
  BEGINGP("MagnetoBomb");
  mPush();
  gl32emu::setShaderEmulation(gl32emu::SE_simpleColour);
  float radius=CORE_RADIUS;
  float maxrad=SPIKE_MAX_RADIUS;
  //No longer used
  //float speed=SPIKE_MOVE_SPEED;

  //Update effects first
  rotation+=ROTATION_SPEED*currentFrameTime;
  if (rotation>2*pi) rotation-=2*pi;

  glTranslatef(x, y, 0);
  glRotatef(rotation/pi*180.0f, 0, 0, 1);
  //We draw the core with triangles, each with a vertex at the centre, the
  //middle of a spike, and the side of a spike. The spikes are drawn in two
  //pieces: The middle and inner edges, which are solid, and the end, which is
  //fully transparent.
  glBegin(GL_TRIANGLES);
  float edges[3][2];
  float angleInc=1.0f/NUM_MAGNETO_GLARES*2*pi;
  for (int i=0; i<=NUM_MAGNETO_GLARES; ++i) {
    float angle=i/(float)NUM_MAGNETO_GLARES*2*pi;
    edges[0][0]=cos(angle)*radius;
    edges[0][1]=sin(angle)*radius;
    edges[1][0]=cos(angle+angleInc/2)*radius;
    edges[1][1]=sin(angle+angleInc/2)*radius;
    edges[2][0]=cos(angle+angleInc)*radius;
    edges[2][1]=sin(angle+angleInc)*radius;
    glColor4f(r, g, b, 1);
    glVertex2f(0,0);
    glVertex2fv(edges[0]);
    glVertex2fv(edges[1]);

    glVertex2fv(edges[0]);
    glVertex2fv(edges[1]);
    if (generalAlphaBlending)
      glColor4f(r, g, b, 0);
    else
      glColor3f(r,g,b);
    glVertex2f(cos(angle+angleInc/2)*maxrad,
               sin(angle+angleInc/2)*maxrad);

    glVertex2f(cos(angle+angleInc/2)*maxrad,
               sin(angle+angleInc/2)*maxrad);
    glColor4f(r, g, b, 1);
    glVertex2fv(edges[2]);
    glVertex2fv(edges[1]);

    glVertex2fv(edges[2]);
    glVertex2fv(edges[1]);
    glVertex2f(0,0);
  }
  glEnd();
  mPop();
  ENDGP
#endif /* defined(AB_OPENGL_14) */
}

CollisionResult MagnetoBomb::checkCollision(GameObject* that) noth {
  if (isRemote) return NoCollision;
  if (armTime>0 && that==parent) return NoCollision;
  if (that->isCollideable())
    return (objectsCollide(this, that)? YesCollision : NoCollision);
  else return UnlikelyCollision;
}

bool MagnetoBomb::collideWith(GameObject* that) noth {
  if (that==this) return false;
  else if (inParentsShields && that==parent) {
    hitParentsShields=true;
    return true;
  } else {
    //Match velocities
    this->vx=that->getVX();
    this->vy=that->getVY();
    explode();
  }
  return false;
}

float MagnetoBomb::getRotation() const noth {
  return rotation;
}

float MagnetoBomb::getRadius() const noth {
  return SPIKE_MAX_RADIUS;
}

bool MagnetoBomb::isCollideable() const noth {
  return false;// true;
}

void MagnetoBomb::explode() noth {
  exploded=true;
  Explosion ex(field, Explosion::Spark, r, g, b, CORE_RADIUS*30,
               0.0001f*power, 1500.0f, this);
  ex.multiExplosion(3);
  if (!isRemote)
    field->inject(new Blast(field, blame, x, y,
                            STD_CELL_SZ*sqrt(power/2)-STD_CELL_SZ/2,
                            power/8.0f,
                            false, CORE_RADIUS+STD_CELL_SZ/2));
}

void MagnetoBomb::simulateFailure() noth {
  armTime=halflife=-1;
}
