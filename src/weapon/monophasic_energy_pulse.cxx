/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/monophasic_energy_pulse.hxx
 */

/*
 * monophasic_energy_pulse.cxx
 *
 *  Created on: 12.02.2011
 *      Author: jason
 */

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "monophasic_energy_pulse.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/collision.hxx"
#include "src/background/explosion.hxx"
#include "src/sim/blast.hxx"
#include "src/ship/ship.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/square.hxx"
#include "src/globals.hxx"

using namespace std;

#define MAX_LIFE_WAVES 512
#define POWER_MUL MONO_POWER_MUL
#define FREQUENCY 0.12f
#define SIZE 0.01f
#define SPEED_MUL 0.0003f

#ifndef AB_OPENGL_14
struct MEPUniform {
  float visibility;
  vec3 baseColour;
};

#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE MEPUniform
DELAY_SHADER(monophasePulse)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFLOAT(visibility), UNIFORM(baseColour), NULL
END_DELAY_SHADER(static mepShader);
#else /* defined(AB_OPENGL_14) */
struct MEPUniform {
  float visibility;
  vec3 colour;
};
#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE MEPUniform
#define baseColour colour;
#define monophasePulseQuick quick
DELAY_SHADER(monophasePulseQuick)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), NULL,
  true,
  UNIFORM(colour), NULL
END_DELAY_SHADER(static mepShader);
#endif /* defined(AB_OPENGL_14) */

MonophasicEnergyPulse::MonophasicEnergyPulse(GameField* field, Ship* par, float x, float y, float theta, unsigned el)
: GameObject(field, x, y, par->getVX() + getSpeed(el)*cos(theta), par->getVY() + getSpeed(el)*sin(theta)),
  deathWaveNumber(rand()%MAX_LIFE_WAVES+1), timeAlive(0), power(el*POWER_MUL),
  previousFrameWasFinal(false), parent(par), exploded(false), blame(par->blame)
{
  classification = GameObject::LightWeapon;
  includeInCollisionDetection=false;
  colrect.radius=SIZE*1.5f;
  collisionBounds.push_back(&colrect);

  isExportable=true;
}

MonophasicEnergyPulse::MonophasicEnergyPulse(GameField* field, float x, float y,
                                             float vx, float vy, unsigned t, unsigned el)
: GameObject(field, x, y, vx, vy),
  deathWaveNumber(100), timeAlive(t), power(el*POWER_MUL),
  previousFrameWasFinal(false), parent(NULL), exploded(false),
  blame(0xFFFFFF)
{
  classification = GameObject::LightWeapon;
  includeInCollisionDetection=false;
  isExportable=true;
  isRemote=true;
  colrect.radius=SIZE*1.5f;
  collisionBounds.push_back(&colrect);
}

bool MonophasicEnergyPulse::update(float et) noth {
  x += vx*et;
  y += vy*et;

  if (isRemote) {
    REMOTE_XYCK;
  }

  if (previousFrameWasFinal) includeInCollisionDetection=false;
  previousFrameWasFinal = currentVFrameLast || decorative;

  float oldTime=timeAlive;
  timeAlive += et;

  float oldAngle = fmod(oldTime*FREQUENCY*2*pi, 2*pi);
  float newAngle = fmod(timeAlive*FREQUENCY*2*pi, 2*pi);
  unsigned wavesAlive = (unsigned)2*timeAlive*FREQUENCY;
  if (wavesAlive == deathWaveNumber) return false;

  //Check if we need to become collideable
  if (!isRemote) {
    if ((oldAngle < pi/2 && newAngle >= pi/2)
    ||  (oldAngle < 3*pi/2 && newAngle >= 3*pi/2)) {
      includeInCollisionDetection=true;
      //Update collision bounds
      colrect.vertices[0] = make_pair(x-SIZE/2, y-SIZE/2);
      colrect.vertices[1] = make_pair(x+SIZE/2, y-SIZE/2);
      colrect.vertices[2] = make_pair(x+SIZE/2, y+SIZE/2);
      colrect.vertices[3] = make_pair(x-SIZE/2, y+SIZE/2);
    }
  }

  return true;
}

void MonophasicEnergyPulse::draw() noth {
  BEGINGP("MonophasicEP")
  float angle = timeAlive*FREQUENCY*2*pi;
  MEPUniform uni = {  1.0f,
                   {{ fabs(sin(8*angle)),
                      fabs(sin(4*angle)),
                      fabs(sin(2*angle))*0.25f+0.75f }}};
  square_graphic::bind();
  mPush();
  mTrans(x,y);
  mUScale(SIZE);
  //Draw a bit larger if zoomed out too much
  if (cameraZoom < 0.3f)
    mUScale(2.0f);
  mTrans(-0.5f,-0.5f);
  mepShader->activate(&uni);
  square_graphic::draw();
  mPop();
  ENDGP
}

float MonophasicEnergyPulse::getRadius() const noth {
  return SIZE/2*1.5f;
}

CollisionResult MonophasicEnergyPulse::checkCollision(GameObject* obj) noth {
  //We never collide with anything if we are remote
  if (obj==parent || isRemote) return NoCollision;
  if (!obj->isCollideable()) return UnlikelyCollision;
  if (objectsCollide(this, obj))
    return YesCollision;
  else
    return UnlikelyCollision;
}

void MonophasicEnergyPulse::explode(GameObject* obj) noth {
  if (EXPCLOSE(x,y))
    field->add(new Explosion(field, Explosion::Simple, 0.3f, 0.5f, 1.0f,
                             1.0f, 0.01f, 1000,
                             x, y, obj->getVX(), obj->getVY()));
  exploded=true;
}

bool MonophasicEnergyPulse::collideWith(GameObject* obj) noth {
  if (obj != this) {
    field->inject(new Blast(field, blame, x, y, SIZE*5, power, true, 0));
    explode(obj);
  }
  return false;
}

float MonophasicEnergyPulse::getWavelength(unsigned level) noth {
  return getSpeed(level)/FREQUENCY;
}

float MonophasicEnergyPulse::getSurvivalProbability(unsigned level, float dist) noth {
  float waves = dist / getWavelength(level);
  return 1.0f - waves/MAX_LIFE_WAVES;
}

float MonophasicEnergyPulse::getSpeed(unsigned level) noth {
  return SPEED_MUL * level;
}
