/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/bussard_ramjet.hxx
 */

#include <cmath>
#include <GL/gl.h>

#include "bussard_ramjet.hxx"
#include "src/ship/sys/engine.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/globals.hxx"
#include "src/core/lxn.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

#define POWER_USAGE 40
#define MAX_THRUST (1200.0f/1000.0f/1000.0f)
#define EXP_SIZE 0.3f
#define EXP_LIFE 500
#define EXP_SPEED 0.0015f
#define EXP_DENSITY 0.075f/1000.0f
#define ROTMUL 0.4f

BussardRamjet::BussardRamjet(Ship* s)
: Engine(s, system_texture::bussardRamjet, ROTMUL)
{
  powerUsageAmt=POWER_USAGE;
  expSize=EXP_SIZE;
  expLife=EXP_LIFE;
  expSpeed=EXP_SPEED;
  expDensity=EXP_DENSITY;
  maxThrust=MAX_THRUST;
  //Primarily red, but other colours mixed in
  rm=0.1f; rb=0.9f;
  gm=0.5f; gb=0.25f;
  bm=0.5f; bb=0.1f;

  SL10N(mountingCondition,engine_mounting_conditions,bussard_ramjet)
}

unsigned BussardRamjet::mass() const noth {
  return 25;
}

bool BussardRamjet::acceptCellFace(float angle) noth {
  float diff=fabs(backT-angle-pi);
  if (diff>2*pi) diff-=2*pi;

  return diff <= pi/4 + 0.001f;
}

float BussardRamjet::thrust() const noth {
  return MAX_THRUST;
}

void BussardRamjet::audio_register() const noth {
  audio::bussardRamjet.addSource(container);
}
