/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/auxobj/plasma_fire.hxx
 */

#include <cmath>
#include <cstdlib>
#include <utility>

#include "plasma_fire.hxx"
#include "src/ship/ship.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/sim/game_field.hxx"

using namespace std;

#define TOTAL_TIME 5000
#define MIN_TIME_BETWEEN_EXPLOSIONS 25

PlasmaFire::PlasmaFire(EmptyCell* cell)
//Like Shield, we keep our "location" at that of the ship
//so we can exist out of bounds
: GameObject(cell->parent->getField(), cell->parent->getX(), cell->parent->getY()), fix(cell),
//Just mark angle so that we can determine it after the cell has been oriented
  angle(-100), timeLeft(TOTAL_TIME - (TOTAL_TIME/2)*(rand()/(float)RAND_MAX)),
  timeSinceLastExplosion(999)
{
  decorative=true;
  okToDecorate();
  includeInCollisionDetection=false;
  fix->fire=this;
}

PlasmaFire::~PlasmaFire() {
  if (fix) fix->fire=NULL;
}

bool PlasmaFire::update(float et) noth {
  if (!fix) return false;

  if (angle == -100)
    angle = atan2(fix->getY() - fix->neighbours[0]->getY(),
                  fix->getX() - fix->neighbours[0]->getX());

  pair<float,float> coord=Ship::cellCoord(fix->parent, fix);
  x = fix->parent->getX();
  y = fix->parent->getY();

  timeLeft -= et / (fix->parent->hasPower()? 3 : 1);
  if (timeLeft <= 0) return false;

  if (!EXPCLOSE(coord.first, coord.second)) return false;

  timeSinceLastExplosion += et;
  if (timeSinceLastExplosion < MIN_TIME_BETWEEN_EXPLOSIONS) return true;
  timeSinceLastExplosion=0;

  pair<float,float> vel=fix->parent->getCellVelocity(fix);
  float power = (fix->parent->hasPower()? 0.5f : 1.0f)*(timeLeft/TOTAL_TIME);
  //Random "billows"
  if (rand()/(float)RAND_MAX < 0.05f) power *= 1 + 4*(rand()/(float)RAND_MAX);

  float desaturation = rand()/(float)RAND_MAX;
  float colour[3] = { 1, 0.5f + desaturation/2, desaturation };
  field->addBegin(new Explosion(field, (rand() & 3? Explosion::Simple : Explosion::Flame),
                                colour[0], colour[1], colour[2],
                                0.100f*power, //sizeAt1Sec
                                0.003f*power, //density
                                2500*power, //lifetime
                                coord.first, coord.second,
                                vel.first  + cos(angle+fix->parent->getRotation())*power*0.00005f,
                                vel.second + sin(angle+fix->parent->getRotation())*power*0.00005f));
  return true;
}
