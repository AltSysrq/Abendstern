/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_emotion.hxx
 */

/*
 * ci_emotion.cxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#include <string>
#include <map>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "src/globals.hxx"
#include "src/sim/game_field.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"
#include "ci_emotion.hxx"

using namespace std;

//Function to pass to lower_bound, using GameField's ordering
//on x right bound
static bool compareObjects(const GameObject* a, const GameObject* b) {
  return a->getX() + a->getRadius() < b->getX() + b->getRadius();
}

cortex_input::EmotionSource::EmotionSource(Ship* s)
: timeUntilRescan(0), field(s->getField()), esship(s)
{
  memset(inputs, 0, sizeof(inputs));
  memset(cumulativeDamage, 0, sizeof(cumulativeDamage));
}

void cortex_input::EmotionSource::getInputs(float* dst) const noth {
  memcpy(dst, inputs, sizeof(inputs));
}

void cortex_input::EmotionSource::update(float et) noth {
  //Reduce temporary pain
  inputs[InputOffset::painta] *= pow(0.5f, et/2048.0f);
  //Determine maximum persistent pain
  const float* maxpp = max_element(cumulativeDamage, cumulativeDamage+lenof(cumulativeDamage));
  unsigned maxppix = maxpp - cumulativeDamage;
  inputs[InputOffset::painpa] = *maxpp;
  inputs[InputOffset::painpt] = maxppix*2*pi/8.0f - pi + pi/8;

  //Rescan nervous and fear if time indicates to do so
  timeUntilRescan -= et;
  if (timeUntilRescan <= 0) {
    timeUntilRescan = rand()%1024 + 1024; //1..2 second interval
    inputs[InputOffset::nervous] = 0;
    inputs[InputOffset::fear] = 0;
    float maxNervous = 0, maxFear = 0;

    GameField::iterator begin = field->begin(),
                        end   = field->end();
    GameField::iterator base =
        lower_bound(begin, end, esship, compareObjects);
    //Look at all objects within 3 screens (rectangle)
    float minx = esship->getX()-3, maxx = esship->getX()+3,
          miny = esship->getY()-3, maxy = esship->getY()+3;
    //Move base to minx
    if (base == end && base != begin) --base;
    while (base != begin && (*base)->getX() >= minx) --base;
    //Sum the parts for nervous and fear.
    //  nervous: for enemy ships, add mass*adot/distanceSquared
    //    where adot is dx*cos(theta)+dy*sin(theta)
    //  fear: for weapons, add threat*vdot/distanceSquared
    //    where threat is 1 for LightWeapon and 2 for HeavyWeapon,
    //    and vdot is dx*vx + dy*vy
    for (GameField::iterator it=base; it != end && (*it)->getX() < maxx; ++it) {
      if ((*it)->getY() >= miny && (*it)->getY() < maxy) {
        if ((*it)->getClassification() == GameObject::ClassShip) {
          Ship* s = (Ship*)*it;
          if (s->hasPower() && Enemies == getAlliance(s->insignia,esship->insignia)) {
            float dx = s->getX() - esship->getX();
            float dy = s->getY() - esship->getY();
            float theta = s->getRotation();
            float amt = max(0.0f, s->getMass()*(dx*cos(theta)+dy*sin(theta))/(dx*dx+dy*dy));
            inputs[InputOffset::nervous] += amt;
            if (amt > maxNervous) {
              maxNervous = amt;
              inputs[InputOffset::nervoust] = atan2(dy,dx);
            }
          }
        } else if ((*it)->getClassification() == GameObject::LightWeapon
               ||  (*it)->getClassification() == GameObject::HeavyWeapon) {
          GameObject* go = *it;
          float dx = go->getX() - esship->getX();
          float dy = go->getY() - esship->getY();
          float threat = (go->getClassification() == GameObject::LightWeapon?
                          1 : 2);
          float amt = max(0.0f, threat*(dx*go->getVX() + dy*go->getVY())/(dx*dx+dy*dy));
          inputs[InputOffset::fear] += amt;
          if (amt > maxFear) {
            maxFear = amt;
            inputs[InputOffset::feart] = atan2(dy,dx);
          }
        }
      }
    }
  }
}

void cortex_input::EmotionSource::damage(float amt, float xoff, float yoff) noth {
  float angle = atan2(yoff,xoff);
  //Add to persistent pain
  if (angle < 0) {
    if (angle < -pi/2) {
      if (angle < -3*pi/4)     cumulativeDamage[0] += amt;
      else                     cumulativeDamage[1] += amt;
    } else {
      if (angle < -pi/4)       cumulativeDamage[2] += amt;
      else                     cumulativeDamage[3] += amt;
    }
  } else {
    if (angle < pi/2) {
      if (angle < pi/4)        cumulativeDamage[4] += amt;
      else                     cumulativeDamage[5] += amt;
    } else {
      if (angle < 3*pi/4)      cumulativeDamage[6] += amt;
      else                     cumulativeDamage[7] += amt;
    }
  }

  //Replace temporary pain if greater
  if (amt > inputs[InputOffset::painta]) {
    inputs[InputOffset::painta] = amt;
    inputs[InputOffset::paintt] = angle;
  }
}
