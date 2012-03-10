/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/energy_charge.hxx
 */

#include <cmath>
#include <iostream>
#include <typeinfo>
#include <GL/gl.h>

#include "energy_charge.hxx"
#include "src/sim/collision.hxx"
#include "src/globals.hxx"
#include "src/background/explosion.hxx"
#include "src/sim/blast.hxx"
//for STD_CELL_SZ
#include "src/ship/cell/cell.hxx"
#include "src/ship/auxobj/shield.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/gl32emu.hxx"

using namespace std;

#define SPEED EC_SPEED
#define RADW EC_RADW
#define RADH EC_RADH
#define DEGREDATION EC_DEGREDATION
#define ALPHA 0.7f

#define VERTEX_TYPE ECVertex
#define UNIFORM_TYPE ECUniform

struct ECVertex {
  vec4 vertex;
  vec2 texCoord;
};
struct ECUniform {
  vec4 colour;
};

#ifndef AB_OPENGL_14
DELAY_SHADER(energyCharge)
  sizeof(ECVertex),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFORM(colour), NULL
END_DELAY_SHADER(static ecshader);
#else
//This is a bit awkward, but for MSVC's sake we need a unique
//class name, but we need the SE name to map to something
//it understands as well.
#define energyChargeQuick quick
DELAY_SHADER(energyChargeQuick)
  sizeof(ECVertex),
  VATTRIB(vertex), NULL,
  true,
  UNIFORM(colour), NULL
END_DELAY_SHADER(static ecshader);
#endif


static GLuint vao,vbo;

//"Texture coordinates" are actually distances from centre
static const ECVertex vertices[4] = {
    { {{+RADW,0,0,1}}, {{+1,0}} },
    { {{0,+RADH,0,1}}, {{0,+1}} },
    { {{0,-RADH,0,1}}, {{0,-1}} },
    { {{-RADW,0,0,1}}, {{-1,0}} },
};

//0-2 G+
//2-4 R-
//4-6 B+
//6-8 G-
//8-0 R+
float EnergyCharge::getColourA(float intensity) noth {
  if (intensity<0.2f) return 0.5f+intensity/0.2f/2;
  else return 1;
}
float EnergyCharge::getColourR(float intensity) noth {
  if (intensity<0.2f) return 1;
  else if (intensity<0.4f) return 1-(intensity-0.2f)*5;
  else if (intensity<0.8f) return 0;
  else return (intensity-0.8f)*5;
}
float EnergyCharge::getColourG(float intensity) noth {
  if (intensity<0.2f) return intensity*5;
  else if (intensity<0.6f) return 1;
  else if (intensity<0.8f) return 1-(intensity-0.6f)*5;
  else return 0;
}
float EnergyCharge::getColourB(float intensity) noth {
  if (intensity<0.4f) return 0;
  else if (intensity<0.6f) return (intensity-0.4f)*5;
  else return 1;
}
float EnergyCharge::getIntensityAt(float initIntensity, float dist) noth {
  float t = dist/SPEED;
  return initIntensity - DEGREDATION*t;
}

static void initVAO() {
  static bool hasVAO=false;
  if (!hasVAO && !headless) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    ecshader->setupVBO();
    hasVAO=true;
  }
}

EnergyCharge::EnergyCharge(GameField* field, const Ship* par, float _x, float _y, float _theta, float power) :
  GameObject(field, _x, _y, par->getVX()+SPEED*cos(_theta), par->getVY()+SPEED*sin(_theta)),
  explodeListeners(NULL),
  parent(par), intensity(power), theta(_theta), tcos(cos(theta)), tsin(sin(theta)),
  exploded(false), blame(par->blame)
{
  classification = GameObject::LightWeapon;
  collisionRectangle.radius=EC_CRRAD;
  isExportable=true;
  collisionBounds.push_back(&collisionRectangle);
  initVAO();
}

EnergyCharge::EnergyCharge(GameField* field, float x, float y,
                           float vx, float vy, float _theta, float _inten)
: GameObject(field, x, y, vx, vy),
  explodeListeners(NULL), parent(NULL),
  intensity(_inten), theta(_theta),
  tcos(cos(theta)), tsin(sin(theta)), exploded(false), blame(-1)
{
  classification = GameObject::LightWeapon;
  collisionRectangle.radius=EC_CRRAD;
  isExportable=true;
  includeInCollisionDetection=false;
  decorative=true;
  isRemote=true;
  collisionBounds.push_back(&collisionRectangle);
  initVAO();
}

EnergyCharge::~EnergyCharge() {
  //Remove any ExplodeListener list we may yet have
  if (explodeListeners)
    explodeListeners->prv = NULL;
}

bool EnergyCharge::update(float et) noth {
  x+=vx*et;
  y+=vy*et;
  intensity-=DEGREDATION*et;
  if (isRemote) {
    //We must NOT go out of bounds or die
    REMOTE_XYCK;
    if (intensity < 0.001f) intensity = 0.001f;
  }
  collisionRectangle.vertices[0].first =x - RADH*tsin;
  collisionRectangle.vertices[0].second=y + RADH*tcos;
  collisionRectangle.vertices[1].first =x - RADW*tcos;
  collisionRectangle.vertices[1].second=y - RADW*tsin;
  collisionRectangle.vertices[2].first =x + RADH*tsin;
  collisionRectangle.vertices[2].second=y - RADH*tcos;
  collisionRectangle.vertices[3].first =x + RADW*tcos;
  collisionRectangle.vertices[3].second=y + RADW*tsin;
  return intensity>0;
}

void EnergyCharge::draw() noth {
  BEGINGP("EnergyCharge")
  float r=getColourR(intensity), g=getColourG(intensity),
        b=getColourB(intensity), a=getColourA(intensity);
  mPush();
  mTrans(x,y);
  mRot(theta);
  //If the zoom is beyond 0.3, we need to increase the size
  //a bit so the charges stay visible
  if (cameraZoom < 0.3)
    mUScale(2.5f);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  ECUniform uni = {{{r,g,b,a}}};
  ecshader->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  mPop();
  ENDGP
}

CollisionResult EnergyCharge::checkCollision(GameObject* obj) noth {
  //We never collide with anything if we are remote
  if (obj==parent || isRemote) return NoCollision;
  if (!obj->isCollideable()) return UnlikelyCollision;
//  if (typeid(*obj)==typeid(Shield) && ((Shield*)obj)->getShip()==parent) return NoCollision;
  if (objectsCollide(this, obj))
    return YesCollision;
  else
    return UnlikelyCollision;
}

bool EnergyCharge::collideWith(GameObject* other) noth {
  if (!other->isCollideable()) return false;
  explode(other);
  field->inject(new Blast(field, blame, x, y, STD_CELL_SZ, 0.1f+intensity*7, true, RADH));
  return false;
}

void EnergyCharge::explode(GameObject* other) noth {
  exploded=true;
  if (!other) other = this;
  Explosion ex(field, Explosion::Simple,
               getColourR(intensity),
               getColourG(intensity),
               getColourB(intensity), 0.15f*(1+intensity),
               0.001f*(1+intensity*10), 1000.0f,
               x, y,
               other->getVX(), other->getVY());
  ex.multiExplosion(1);

  //Set velocity to match other for networking purposes
  vx = other->getVX();
  vy = other->getVY();

  for (ExplodeListener<EnergyCharge>* l = explodeListeners; l; l = l->nxt)
    l->exploded(this);
}

float EnergyCharge::getRotation() const noth {
  return theta;
}

float EnergyCharge::getRadius() const noth {
  return RADW;
}
