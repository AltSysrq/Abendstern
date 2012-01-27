/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_navigation/aim_nav_to_target.hxx
 */

/*
 * aim_nav_to_target.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <cmath>
#include <iostream>

#include "aim_nav_to_target.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"

using namespace std;

AIM_NavToTarget::AIM_NavToTarget(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  cruisingSpeed(s.exists("cruising_speed")? s["cruising_speed"] : 2.5f)
{ }

void AIM_NavToTarget::action() {
  Ship* target = (Ship*)ship->target.ref;
  if (!target) return;

  /* Derivation of the code below:
   * P = location of ship
   * v = velocity of ship
   * a = acceleration of ship in correct direction, given throttle
   * Q = location of target
   * w = velocity of target
   * R = extrapolation of location of target at time of intercept
   * t = time
   *
   * R = Q + wt
   * (for intercept)
   * R = P + vt + 1/2*att
   *
   * Q + wt = P + vt + 1/2*att
   * 1/2*att + (v-w)t + (P-Q) = 0
   * (quadratic formula; magnitudes for vectors)
   * t = (-(v-w) +/- sqrt((v-w)^2 - 4*(1/2*a)*(P-Q)))/(2*1/2*a)
   *   = (w - v +/- sqrt(vv - 2vw + ww - 2aP + 2aQ))/a
   * We only care about the value of the sqrt (the contents /can/
   * be negative), so take the absolute value before sqrting so
   * we get real answers.
   * We use the absolute value of t that is lower, and cap it at
   * beh.foresight.
   *
   * We then use that t value to extrapolate the position of the target
   * and return that.
   *
   *
   * Additionally, we need to be able to work when a=0. In that case,
   * R = Q + wt = P + vt
   * Q - P = vt - wt
   * Q - P = t(v-w)
   * t = (Q - P)/(v-w)
   * In this case, we use which ever component is greater for v-w.
   */
  float t;
  if (ship->getAcceleration() != 0) {
    float a = ship->getAcceleration();
    float v = sqrt(ship->getVX()*ship->getVX() + ship->getVY()*ship->getVY());
    float w = sqrt(target->getVX()*target->getVX() + target->getVY()*target->getVY());
    float p = sqrt(ship->getX()*ship->getX() + ship->getY()*ship->getY());
    float q = sqrt(target->getX()*target->getX() + target->getY()*target->getY());
    float inner = sqrt(fabs(v*v - 2*v*w + w*w - 2*a*p + 2*a*q));
    float t1 = fabs((w - v + inner)/a);
    float t2 = fabs((w - v - inner)/a);
    t = (t1 < t2? t1:t2);
  } else {
    if (fabs(ship->getVX()-target->getVX()) > fabs(ship->getVY()-target->getVY())) {
      if (0 != ship->getVX() - target->getVX())
        t = (target->getX() - ship->getX())/(ship->getVX() - target->getVX());
      else
        t = 10000;
    } else {
      if (0 != ship->getVY() - target->getVY())
        t = (target->getY() - ship->getY())/(ship->getVY() - target->getVY());
      else
        t = 10000;
    }
    if (t<0) t=10000;
  }

  if (t > 10000) t=10000;
  float tx = target->getX() + target->getVX()*t,
        ty = target->getY() + target->getVY()*t;

  float dx = tx - ship->getX();
  float dy = ty - ship->getY();
  float speed = sqrt(ship->getVX()*ship->getVX() + ship->getVY()*ship->getVY());
  float tangle = atan2(dy, dx),
        cangle = ship->getRotation()+controller.gglob("thrust_angle", 0.0f),
        vangle = (speed > 0.0001f? atan2(ship->getVY(), ship->getVX()) : cangle);
  float cangDiff = tangle - cangle,
        vangDiff = tangle - vangle;
  if (cangDiff<-pi) cangDiff+=2*pi;
  if (cangDiff>+pi) cangDiff-=2*pi;
  if (vangDiff<-pi) vangDiff+=2*pi;
  if (vangDiff>+pi) vangDiff-=2*pi;
  controller.setTargetTheta(tangle+vangDiff/2-controller.gglob("thrust_angle", 0.0f));
  if (speed > cruisingSpeed) ship->configureEngines(false, true, 0.05f);
  else {
    if      (fabs(cangDiff) > pi/2)  ship->configureEngines(false, true, controller.gglob("max_brake", 1.0f));
    else if (fabs(cangDiff) > pi/4)  ship->configureEngines(true,  true, controller.gglob("max_throttle", 1.0f)/2);
    else                             ship->configureEngines(true, false, controller.gglob("max_throttle", 1.0f));
  }
}

static AIModuleRegistrar<AIM_NavToTarget> registrar("navigation/target");
