/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/semiguided_bomb.hxx
 */

#include <cmath>

#include "semiguided_bomb.hxx"
#include "src/ship/cell/cell.hxx" //For STD_CELL_SZ
#include "src/globals.hxx"

using namespace std;


#define MAX_GUIDANCE (1/1000.0f/1000.0f)
#define MAX_SPEED 0.00075f
#define GUIDANCE_ACTIVATE_TIME 1500.0f


bool SemiguidedBomb::update(float et) noth {
  if (!coreUpdate(et)) return false;
  if (!currentVFrameLast) return true;
  et = currentFrameTime;

  Ship* parent = (Ship*)this->parent.ref;

  if (!parent || !parent->hasPower() || !parent->target.ref) return true; //No velocity change possible
  Ship* target = (Ship*)parent->target.ref;

  float tbdx = target->getX() - x,
        tbdy = target->getY() - y;
  float tbdist = sqrt(tbdx*tbdx + tbdy*tbdy);

  float cosine = tbdx/tbdist, sine = tbdy/tbdist;
  if (tbdist < 0.01f) tbdist=0.01f;
  //Since most distances will be <1, and we want sqrt to reduce
  //the value, do sqrt(pbdist+1)-1
  float guidanceActivated = (timeAlive < GUIDANCE_ACTIVATE_TIME? timeAlive/GUIDANCE_ACTIVATE_TIME : 1);
  float guidance = MAX_GUIDANCE/tbdist * et * guidanceActivated;
  vx += guidance*cosine;
  vy += guidance*sine;

/*  float dvx = vx-parent->getVX(), dvy = vy-parent->getVY();
  float speedsq = dvx*dvx + dvy*dvy;
  if (speedsq > MAX_SPEED*MAX_SPEED) {
    float speed = sqrt(speedsq);
    vx = parent->getVX() + dvx/speed*MAX_SPEED;
    vy = parent->getVY() + dvy/speed*MAX_SPEED;
  }*/
  return true;
}
