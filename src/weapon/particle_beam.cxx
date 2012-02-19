/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/particle_beam.hxx
 */

/*
 * particle_beam.cxx
 *
 *  Created on: 09.03.2011
 *      Author: jason
 */

#include <vector>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>

#include <GL/gl.h>

#include "particle_beam.hxx"
#include "src/sim/collision.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/globals.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"

#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"

#include "src/exit_conditions.hxx"

#define RADIUS (STD_CELL_SZ/4)
#define PARTICLE_SPEED 0.001f //1 screen/sec
#define PARTICLE_LIFE 512
#define PARTICLE_COLLISION_LIFE 20

using namespace std;

ParticleEmitter::ParticleEmitter(GameField* field, ParticleBeamType t,
                                 float x, float y, float vx, float vy,
                                 Ship* par)
: GameObject(field, x, y, vx, vy),
  parent(par), type(t), rmajor(0), rminor(1), timeAlive(0),
  blame(par->blame)
{
  collisionBounds.push_back(&colrect);
  classification = GameObject::LightWeapon;
  colrect.radius = RADIUS;
  decorative=true; //No sense in not being, since we only spawn once/128 ms
  okToDecorate();

  isExportable=true;

  for (unsigned i=0; i<r_sz; ++i)
    r[i] = rand();
}

ParticleEmitter::ParticleEmitter(GameField* field, ParticleBeamType t, unsigned bla,
                                 float x, float y, float vx, float vy,
                                 const unsigned char* _r,
                                 unsigned rmaj, unsigned rmin,
                                 float ta,
                                 Ship* par)
: GameObject(field, x, y, vx, vy),
  parent(par), type(t), rmajor(rmaj), rminor(rmin), timeAlive(ta),
  blame(bla)
{
  collisionBounds.push_back(&colrect);
  classification = GameObject::LightWeapon;
  colrect.radius = RADIUS;
  decorative=true; //No sense in not being, since we only spawn once/128 ms
  okToDecorate();

  isExportable=true;
  isRemote=true;
  includeInCollisionDetection=false;

  memcpy(r, _r, sizeof(r));
}

bool ParticleEmitter::update(float et) noth {
  x += vx*et;
  y += vy*et;
  if (isRemote) {
    REMOTE_XYCK;
  }

  float newTimeAlive = timeAlive + et;
  unsigned oldN = (unsigned)(timeAlive / emissionTime),
           newN = (unsigned)(newTimeAlive / emissionTime);
  timeAlive = newTimeAlive;

  //Since we don't die when remote, we need to make
  //sure we don't invalidate the assumptions in
  //the emission code below
  if (isRemote && oldN >= lifetime/emissionTime) return true;

  if (oldN != newN) {
    //Emit
    float theta = (r[rmajor]^r[rminor])/128.0f*pi;
    ++rminor;
    /* We don't have to worry about the double-increment
     * case (maj=7 min=6 -> maj=7 min=7 -> maj=0 min=0 -> maj=0 min=1).
     * Our life time is 2560, and emission time is 64, so we
     * emit a total of 40 particles, where that case (and wrap-
     * around of rmajor) cannot occur.
     */
    if (rminor == rmajor) ++rminor;
    if (rminor == r_sz) rminor=0, ++rmajor;

    //Spawn
    field->add(new ParticleBurst(field, blame, x, y, cos(theta)*PARTICLE_SPEED, sin(theta)*PARTICLE_SPEED,
                                 0, 0, PARTICLE_LIFE*(timeAlive/lifetime*0.5f+0.5f), type));
  }

  return isRemote || timeAlive <= lifetime;
}

void ParticleEmitter::draw() noth {}

float ParticleEmitter::getRadius() const noth { return RADIUS; }

const vector<CollisionRectangle*>* ParticleEmitter::getCollisionBounds() noth {
  colrect.vertices[0].first = x-RADIUS;
  colrect.vertices[0].second= y-RADIUS;
  colrect.vertices[1].first = x+RADIUS;
  colrect.vertices[1].second= y-RADIUS;
  colrect.vertices[2].first = x+RADIUS;
  colrect.vertices[2].second= y+RADIUS;
  colrect.vertices[3].first = x-RADIUS;
  colrect.vertices[3].second= y+RADIUS;
  return &collisionBounds;
}

CollisionResult ParticleEmitter::checkCollision(GameObject* that) noth {
  if (that == parent) return NoCollision;
  if (!that->isCollideable()) return NoCollision;
  return (objectsCollide(this, that)? YesCollision : NoCollision);
}

bool ParticleEmitter::collideWith(GameObject*) noth {
  return false; //Always just die silently
}


//--------------------------

#ifndef AB_OPENGL_14
static GLuint pbvao, pbvbo;
#endif /* AB_OPENGL_14 */

ParticleBurst::ParticleBurst(GameField* field, unsigned bla,
                             float x, float y, float vx, float vy, float _ovx, float _ovy,
                             float life, ParticleBeamType t)
: GameObject(field, x, y, vx, vy),
  ox(x), oy(y), ovx(_ovx), ovy(_ovy),
  timeLeft(life), hasCollided(false), type(t),
  blame(bla)
{
  collisionBounds.push_back(&colrect);
  classification = GameObject::LightWeapon;
  colrect.radius = RADIUS;

  #ifndef AB_OPENGL_14
    static bool hasVAO=false;
    if (!headless && !hasVAO) {
      pbvao=newVAO();
      glBindVertexArray(pbvao);
      pbvbo=newVBO();
      glBindBuffer(GL_ARRAY_BUFFER, pbvbo);
      const shader::quickV vertices[2] = {
        { {{0.0f,0.0f}} }, { {{1.0f,1.0f}} }
      };
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      shader::quick->setupVBO();
      hasVAO=true;
    }
  #endif /* AB_OPENGL_14 */
}

bool ParticleBurst::update(float et) noth {
  timeLeft -= et;
  ox += ovx*et;
  oy += ovy*et;
  x += vx*et;
  y += vy*et;
  if (timeLeft < 0) {
    vx=ovx;
    vy=ovy;
    decorative=true;
    includeInCollisionDetection=false;
  }
  return timeLeft > -PARTICLE_LIFE;
}

void ParticleBurst::draw() noth {
  float r, g, b, a;
  switch (type) {
    case NeutronBeam:
      r=0.1f;
      g=1.0f;
      b=0.15f;
      break;
    case MuonBeam:
      r=0.7f;
      g=0.8f;
      b=1.0f;
      break;
    case AntiprotonBeam:
      r=1.0f;
      g=0.2f;
      b=0.1f;
      break;
    case PositronBeam:
      r=0.8f;
      g=0.2f;
      b=1.0f;
      break;

    default:
      cerr << __FILE__ <<':'<< __LINE__ << ": bad ParticleBurst type!" << endl;
      exit(EXIT_THE_SKY_IS_FALLING);
  }
  if (timeLeft > 0) a=1.0f;
  else a = 1.0f+timeLeft/PARTICLE_LIFE;

#ifndef AB_OPENGL_14
  glBindVertexArray(pbvao);
  glBindBuffer(GL_ARRAY_BUFFER, pbvbo);
  shader::quickU uni = { {{r,g,b,a}} };
  mPush();
  mTrans((ox+x)/2,(oy+y)/2);
  mScale((x-ox)/2, (y-oy)/2);
  shader::quick->activate(&uni);
  glDrawArrays(GL_LINES, 0, 2);
  mPop();
#else
  glColor4f(r,g,b,a);
  glBegin(GL_LINES);
    glVertex2f((ox+x)/2,(oy+y)/2);
    glVertex2f(x,y);
  glEnd();
#endif
}

float ParticleBurst::getRadius() const noth {
  return RADIUS;
}

const vector<CollisionRectangle*>* ParticleBurst::getCollisionBounds() noth {
  colrect.vertices[0].first = x-RADIUS;
  colrect.vertices[0].second= y-RADIUS;
  colrect.vertices[1].first = x+RADIUS;
  colrect.vertices[1].second= y-RADIUS;
  colrect.vertices[2].first = x+RADIUS;
  colrect.vertices[2].second= y+RADIUS;
  colrect.vertices[3].first = x-RADIUS;
  colrect.vertices[3].second= y+RADIUS;
  return &collisionBounds;
}

CollisionResult ParticleBurst::checkCollision(GameObject* that) noth {
  if (!that->isCollideable()) return NoCollision;
  return (timeLeft>0 && objectsCollide(this, that)? YesCollision : NoCollision);
}

bool ParticleBurst::collideWith(GameObject* go) noth {
  if (!hasCollided) {
    hasCollided = true;
    if (timeLeft > PARTICLE_COLLISION_LIFE)
      timeLeft = PARTICLE_COLLISION_LIFE;
  }
  return go != this;
}
