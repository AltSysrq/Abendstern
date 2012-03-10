/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/missile.hxx
 */

/*
 * missile.cxx
 *
 *  Created on: 07.03.2011
 *      Author: jason
 */

#include <cmath>
#include <vector>
#include <typeinfo>

#include <GL/gl.h>

#include "missile.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/background/explosion.hxx"
#include "src/sim/objdl.hxx"
#include "src/sim/blast.hxx"
#include "src/globals.hxx"
#include "src/sim/collision.hxx"
#include "src/secondary/light_trail.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx" //For STD_CELL_SZ
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/square.hxx"
#include "src/graphics/glhelp.hxx"

//All level-related constants based on level 1

#define LIFETIME 10000.0f
#define DAMAGE_MUL 10.0f
#define ACCELERATION 0.000002f //2 screen/sec/sec
//Lose 3% of speed per second
#define FRICTION 0.00003f
#define RADIUS (STD_CELL_SZ)
#define HOMING_THRESHHOLD 0.95f
#define SQRT2 1.4142136f

using namespace std;

#ifndef AB_OPENGL_14
  struct MisUniform {
    float green;
  };
  #define UNIFORM_TYPE MisUniform
  #define VERTEX_TYPE shader::VertTexc
  DELAY_SHADER(missile)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), VATTRIB(texCoord), NULL,
    true, UNIFLOAT(green), NULL
  END_DELAY_SHADER(static missileShader);
#endif /* not defined AB_OPENGL_14 */

Missile::Missile(GameField* field, int lvl, float x, float y, float vx,
                 float vy, Ship* par, GameObject* tgt)
: GameObject(field, x, y, vx, vy),
  explodeListeners(NULL),
  trail(NULL), target(tgt), parent(par),
  level(lvl), timeAlive(0), exploded(false),
  blame(par->blame),
  ax(0), ay(0), xdir(0), ydir(0)
{
  collisionBounds.push_back(&colrect);
  classification = GameObject::HeavyWeapon;
  colrect.radius = RADIUS;

  isExportable=true;
}

//Networking constructor
Missile::Missile(GameField* field, int lvl, float x, float y, float vx,
                 float vy, float ax_, float ay_, float ta)
: GameObject(field, x, y, vx, vy),
  explodeListeners(NULL),
  trail(NULL), target(NULL), parent(NULL),
  level(lvl), timeAlive(ta), exploded(false), blame(0xFFFFFF),
  ax(ax_), ay(ay_), xdir(0), ydir(0)
{
  isExportable=true;
  isRemote=true;
  includeInCollisionDetection=false;
  decorative=true;
}

Missile::~Missile() {
  //Remove any ExplodeListener chain attached
  if (explodeListeners)
    explodeListeners->prv = NULL;
}

bool Missile::update(float et) noth {
  x += vx*et;
  y += vy*et;
  timeAlive += et;
  if (timeAlive > LIFETIME && !isRemote) {
    explode(this);
    return false;
  }
  if (isRemote) {
    REMOTE_XYCK;
  }

  if (target.ref && currentVFrameLast && !isRemote) {
    GameObject* t=target.ref;
    float accel=ACCELERATION/level*(LIFETIME-timeAlive)/LIFETIME;
    /*
    float dx = x-t->getX()+(vx-t->getVX())*512,
          dy = y-t->getY()+(vy-t->getVY())*512;
    float dist = sqrt(dx*dx + dy*dy);
    dx /= dist;
    dy /= dist;

    //Home in
    vx -= dx*accel*currentFrameTime;
    vy -= dy*accel*currentFrameTime;
    */
    float dx = t->getX()-x, dy = t->getY()-y;
    float dot = vx*dx + vy*dy;

    float speed = sqrt(vx*vx+vy*vy);
    float dist  = sqrt(dx*dx+dy*dy);
    float cross = vx*dy - vy*dx;
    float sign = (cross > 0? -1 : +1);
    ax = -sign*vy/speed*accel;
    ay = +sign*vx/speed*accel;
    if (dot/speed/dist > HOMING_THRESHHOLD || dot < 0) {
      ax = (ax*SQRT2 - SQRT2*dx/dist*accel)/2;
      ay = (ay*SQRT2 - SQRT2*dy/dist*accel)/2;
    }

    vx -= ax*currentFrameTime;
    vy -= ay*currentFrameTime;

    xdir = dx/dist*accel;
    ydir = dy/dist*accel;
  } else if (isRemote) {
    vx += ax*et;
    vy += ay*et;
  }
  if (EXPCLOSE(x,y) && !headless && currentVFrameLast
  &&  (isRemote || target.ref)) {
    if (!trail.ref) {
      trail.assign(new LightTrail(field, 3000, 64, 4*RADIUS, RADIUS,
                                  0.8f, 0.8f, 1.0f, 1.0f,
                                  0.0f, -0.5f, -1.0f, -2.0f));
      field->add(trail.ref);
    }
    LightTrail* trail=(LightTrail*)this->trail.ref;
    trail->emit(x, y, vx-xdir*1000*level, vy-ydir*1000*level);
  }

  //Friction
  //vx -= vx*FRICTION*et/level;
  //vy -= vy*FRICTION*et/level;
  return true;
}

void Missile::draw() noth {
  #ifndef AB_OPENGL_14
    mPush();
    mTrans(x,y);
    mUScale(RADIUS);
    mTrans(-0.5f,-0.5f);
    MisUniform uni = { 1.0f - timeAlive/LIFETIME };
    square_graphic::bind();
    missileShader->activate(&uni);
    square_graphic::draw();
    mPop();
  #else
    glPushMatrix();
    glTranslatef(x, y, 0);
    gl32emu::setShaderEmulation(gl32emu::SE_quick);
    glColor3f(1.0f, 1.0f - timeAlive/LIFETIME, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i=0; i<6; ++i)
      glVertex2f(RADIUS*cos(i/3.0f*pi), RADIUS*sin(i/3.0f*pi));
    glVertex2f(RADIUS, 0);
    glEnd();
    glPopMatrix();
  #endif /* AB_OPENGL_14 */
}

float Missile::getRadius() const noth {
  return RADIUS;
}

CollisionResult Missile::checkCollision(GameObject* go) noth {
  //Don't collide with other missiles
  if (go->getClassification() == HeavyWeapon
  &&  typeid(*go) == typeid(Missile)) return NoCollision;
  if (go != parent.ref && objectsCollide(this, go)) return YesCollision;
  return NoCollision;
}

bool Missile::collideWith(GameObject* go) noth {
  if (go != this) explode(go);
  return false;
}

const vector<CollisionRectangle*>* Missile::getCollisionBounds() noth {
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

void Missile::explode(GameObject* ref) noth {
  if (!ref) ref=this;
  if (!isRemote)
    field->inject(new Blast(field, blame, x, y, STD_CELL_SZ*3.5f,
                            level*DAMAGE_MUL, true, RADIUS));
  if (EXPCLOSE(x,y))
    field->add(new Explosion(field, Explosion::Simple, 0.8f, 0.8f, 1.0f,
                             sqrt((float)level)*2.5f, level/2000.0f,
                             1000, x, y, ref->getVX(), ref->getVY()));

  //Reset velocities to that of ref so that the explosion's velocity
  //can be communicated over the network.
  vx = ref->getVX();
  vy = ref->getVY();

  exploded=true;
  for (ExplodeListener<Missile>* l = explodeListeners; l; l = l->nxt)
    l->exploded(this);
}
