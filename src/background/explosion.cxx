/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/explosion.hxx
 */
#include <cstdlib>
#include <cmath>
#include <iostream>

#include <GL/gl.h>
#include "explosion.hxx"
#include "src/sim/game_field.hxx"
#include "src/globals.hxx"
#include "src/fasttrig.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

static char _staticExplosion[sizeof(Explosion)];
void* staticExplosion((void*)_staticExplosion);

/* Used so that multiExplosion() does not result in tons of calls
 * to the effects handler.
 */
bool callingMultiExplosion=false;
Explosion::Explosion(GameField* field, ExplosionType extype,
                     GLfloat red, GLfloat green, GLfloat blue,
                     float size1Sec, float _density, float life, float _x, float _y,
                     float _vx, float _vy, float _smFac, float _smAng) :
  GameObject(field, _x, _y, _vx, _vy),
  lifetime(life),
  sizeAt1Sec(size1Sec), //list(0),
  density(_density),
  smearFactor(_smFac), smearAngle(_smAng),
  type(extype),
  effectsDensity(_density), hungry(false)
{
  colour[0]=red; colour[1]=green; colour[2]=blue;
}

Explosion::Explosion(GameField* field, ExplosionType extype,
                     GLfloat red, GLfloat green, GLfloat blue,
                     float size1Sec, float _density, float life, GameObject* reference) :
  GameObject(field, reference->getX(), reference->getY(), reference->getVX(), reference->getVY()),
  lifetime(life),
  sizeAt1Sec(size1Sec), //list(makeList(_density)),
  density(_density),
  smearFactor(0), smearAngle(0),
  type(extype),
  effectsDensity(_density), hungry(false)
{
  colour[0]=red; colour[1]=green; colour[2]=blue;
}

Explosion::Explosion(const Explosion& that)
: GameObject(that.field, that.x, that.y, that.vx, that.vy),
  lifetime(that.lifetime), sizeAt1Sec(that.sizeAt1Sec),
  density(that.density),
  smearFactor(that.smearFactor), smearAngle(that.smearAngle),
  type(that.type),
  effectsDensity(that.effectsDensity), hungry(that.hungry)
{
  colour[0]=that.colour[0];
  colour[1]=that.colour[1];
  colour[2]=that.colour[2];
}

bool Explosion::update(float time) noth {
/*  elapsedLife+=time;
  x+=vx*time;
  y+=vy*time;
  return elapsedLife<lifetime; */

  cerr << "Explosion::update is to be handled by GameField" << endl;
  exit(EXIT_PROGRAM_BUG);
  return false;
}

void Explosion::draw() noth {
  /*
  if (list==0) list=makeList(density);

  glPushMatrix();
  glTranslatef(x, y, 0);
  GLfloat scale=(GLfloat)elapsedLife/1000.0f;
  glScalef(scale, scale, 1);
  GLfloat intensity=(lifetime-elapsedLife)/lifetime;
  glColor4f(colour[0], colour[1], colour[2], intensity);
  glCallList(list);
  glPopMatrix();
  */
  cerr << "Explosion::draw is to be handled by GameField" << endl;
}

CollisionResult Explosion::checkCollision(GameObject* other) noth {
  return NoCollision;
}

float Explosion::getRadius() const noth {
  //return (float)elapsedLife/1000.0f*sizeAt1Sec;

  //MSVC++ doesn't like this line, so we'll do it indirectly
  //return 0.0f/0.0f;
  //float a=0; return a/a;
  return NAN;
}

Explosion::~Explosion() {
//  glDeleteLists(list,1);
}

static float randomColourVariation() {
  return 1 + frand()/(float)RAND_MAX*0.4f-0.2f;
}

void Explosion::multiExplosion(int count) noth {
  callingMultiExplosion=true;
  for (int i=0; i<count; ++i) {
    Explosion* ex=new Explosion(field, type,
                                colour[0]*randomColourVariation(),
                                colour[1]*randomColourVariation(),
                                colour[2]*randomColourVariation(),
                                sizeAt1Sec, density/count,
                                lifetime,
                                x, y, vx, vy, smearFactor, smearAngle);
    ex->effectsDensity=effectsDensity;
    field->add(ex);
  }
  callingMultiExplosion=false;
}
