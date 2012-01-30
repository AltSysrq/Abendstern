/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/engine.hxx
 */

#include <cmath>
#include <iostream>
#include <typeinfo>

#include "engine.hxx"
#include "ship_system.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/circle_cell.hxx"
#include "src/globals.hxx"
#include "src/background/explosion.hxx"
#include "src/secondary/light_trail.hxx"
#include "src/core/lxn.hxx"

using namespace std;

/* To be accepted, a Cell MUST:
 * + Not be housing another Engine
 * + Have at least one open side between 90 and 270, not inclusive (id est, the back)
 * + Must NOT be a circle
 */
const char* Engine::autoOrient() noth {
  Cell* cell=container;
  if (typeid(*cell) == typeid(CircleCell)) RETL10N(engine_mounting_conditions,not_circle)
  bool hasBack=false;
  orientation=-1;
  for (unsigned i=0; i<cell->numNeighbours(); ++i) {
    if (cell->neighbours[i]) continue;
    int theta=(int)(cell->getT() + cell->edgeT(i)) % 360;
    if (theta>91 && theta<269) {
      bool hadBack=hasBack;
      hasBack=true;
      float newBackT=theta/360.0f*2*pi;
      /* See if this back is acceptable */
      float oldBackT=backT;
      backT=newBackT;
      bool acceptable=needsNoIntakes();
      for (unsigned j=0; j<cell->numNeighbours(); ++j) {
        if (!cell->neighbours[j]) {
          float jtheta=(cell->getT() + cell->edgeT(j))/180.0f*pi;
          while (jtheta > pi) jtheta -= 2*pi;
          acceptable |= acceptCellFace(jtheta);
        }
      }

      backT=oldBackT;
      if (acceptable && (!hadBack || fabs(newBackT-pi)<fabs(backT-pi))) {
        backT=newBackT;
        orientation = i;
        backX=cell->getX() /*- cell->edgeD(i)*cos(backT)*/;
        backY=cell->getY() /*- cell->edgeD(i)*sin(backT)*/;
      }
    }
  }

  thrustAngle=backT;
  cellT=cell->getT();
  autoRotate=backT-cellT*pi/180+pi;
  if (!hasBack || orientation==-1) return mountingCondition.c_str();

  return NULL;
}

const char* Engine::setOrientation(int i) noth {
  if (i == -1) return autoOrient();
  if (i < -2 || i >= 4) return "Invalid orientation";

  orientation=i;

  Cell* cell=container;
  if (typeid(*cell) == typeid(CircleCell))
    RETL10N(engine_mounting_conditions,not_circle)
  if (cell->neighbours[i])
    RETL10N(engine_mounting_conditions,output_blocked)
  int theta=(int)(cell->getT() + cell->edgeT(i)) % 360;
  if (theta>91 && theta<269) {
    backT=theta/360.0f*2*pi;
    bool acceptable=needsNoIntakes();
    for (unsigned j=0; j<cell->numNeighbours(); ++j) {
      if (!cell->neighbours[j]) {
        float jtheta=(cell->getT() + cell->edgeT(j))/180.0f*pi;
        while (jtheta > pi) jtheta -= 2*pi;
        acceptable |= acceptCellFace(jtheta);
      }
    }

    if (acceptable) {
      backX=cell->getX() /*- cell->edgeD(i)*cos(backT)*/;
      backY=cell->getY() /*- cell->edgeD(i)*sin(backT)*/;
      thrustAngle=backT;
      cellT=cell->getT();
      autoRotate=backT-cellT*pi/180+pi;
      //Success
      return NULL;
    }
  }

  //Failure
  return mountingCondition.c_str();
}

int Engine::getOrientation() const noth {
  return orientation;
}

void Engine::update(Ship* ship, float time) noth {
  float rot=ship->getRotation();
  if (ship->isThrustOn()) ship->engineInfo.physicalOnTime+=time;

  if (ship->engineInfo.physicalOnTime==0) return;
  ship->engineInfo.physicalOnTime=0; //Reset now

  if (ship->engineInfo.fade != 1 && ship->isThrustOn() && ship->hasPower() && !ship->isStealth()) {
    ship->engineInfo.fade += time/1000.0f;
    if (ship->engineInfo.fade>1) ship->engineInfo.fade=1;
  } else if (ship->engineInfo.fade != 0 && (!ship->isThrustOn() || !ship->hasPower())) {
    ship->engineInfo.fade -= time/2000.0f;
    if (ship->engineInfo.fade<0) ship->engineInfo.fade=0;
  }

  //Only add if we are close enough to the screen
  if (!EXPCLOSE(ship->getX(), ship->getY()) || headless) return;
  //pair<float,float> cellVelocity=that.parent->getCellVelocity(that.container);
  //Just approximate for now, since it's faster
  pair<float,float> cellVelocity(ship->getVX(), ship->getVY());

  //Pretend we're a physical frame
  time=currentFrameTime;
  ship->engineInfo.timeSinceLastExpsosion += currentFrameTime;
  //if (ship->engineInfo.timeSinceLastExpsosion < 50) return; //Too dense
  ship->engineInfo.timeSinceLastExpsosion=0;

  //The only stealth-mode engine produces no explosions
  if (!ship->isStealth()) {
    for (unsigned i=0; i<ship->engineInfo.list.size(); ++i) {
      Engine& that=*ship->engineInfo.list[i];
      //No-explosion engines use a negative expSize
      if (that.expSize < 0) continue;

      //Create explosions to simulate accelerated particles
      //At max thrust for one intake, the particles will
      //move at one screen per second, or 0.001 S/ms.
      //Therefore, speed=thrust()/MAX_THRUST*0.001,
      //vx=parent->getVX()-cos(backT)*speed,
      //vy=parent->getVY()-sin(backT)*speed,
      //x=parent->getX()+backX,
      //y=parent->getY()+backY
      //The density is determined similarly to speed;
      //at max thrust for one intake, the density should be
      //0.002*time/1000. The spread value is 0.1, and the lifetime
      //500ms.

//      float t=that.thrust();
//      float speed=that.expSpeed*t/that.maxThrust;
//      float cosine=cos(that.backT+rot),
//            sine=sin(that.backT+rot);
//      pair<float,float> coord=Ship::cellCoord(ship, that.container);
//      float smear=speed*5000;
//      float vx=cellVelocity.first+cosine*speed;
//      float vy=cellVelocity.second+sine*speed;
//      float red=rand()/(float)RAND_MAX*that.rm + that.rb;
//      float grn=rand()/(float)RAND_MAX*that.gm + that.gb;
//      float blu=rand()/(float)RAND_MAX*that.bm + that.bb;
//      Explosion* ex = new Explosion (field,
//                                     Explosion::Simple,
//                                     red, grn, blu,
//                                     that.expSize,
//                                     that.expDensity*5,//currentFrameTime*onPercent*that.parent->getThrust(),
//                                     that.expLife,
//                                     coord.first,
//                                     coord.second,
//                                     vx,
//                                     vy,
//                                     smear,
//                                     that.backT+rot);
//      ex->effectsDensity*=0.1f;
//      field->addBegin(ex);

      float t=that.thrust();
      float speed=ship->getThrust() * that.expSpeed*t/that.maxThrust;
      float cosine=cos(that.backT+rot),
            sine=sin(that.backT+rot);
      pair<float,float> coord=Ship::cellCoord(ship, that.container);
      //No longer used
      //float vx=cellVelocity.first+cosine*speed;
      //float vy=cellVelocity.second+sine*speed;

      if (!that.trail.ref) {
        that.trail.assign(new LightTrail(ship->getField(), that.expLife*2, 96,
                                         that.expSize*2, 0.001,
                                         that.rb, that.gb, that.bb, 1,
                                         0.0f, -1.5f, -3.0f, -2.0f));
        ship->getField()->add(that.trail.ref);
      }

      float rm = 0.5f + rand()/2/(float)RAND_MAX;
      float tvx = cellVelocity.first + cosine*speed*rm;
      float tvy = cellVelocity.second + sine*speed*rm;

      ((LightTrail*)that.trail.ref)->setWidth(that.expSize*2*ship->getThrust());
      ((LightTrail*)that.trail.ref)->emit(coord.first, coord.second, tvx, tvy);
    }
  }
}
