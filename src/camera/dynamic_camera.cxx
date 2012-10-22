/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/dynamic_camera.hxx
 */

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "camera.hxx"
#include "dynamic_camera.hxx"
#include "src/globals.hxx"
#include "src/control/human_controller.hxx"
#include "src/control/hc_conf.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/asgi.hxx"

using namespace std;
using namespace hc_conf;

#define ACCEL_SPEED 0.001f
#define MOVE_SPEED 0.01f
#define ZOOM_SPEED 0.002f
#define STABILIZE_RATIO_SEC 0.1f
#define ROT_SPEED (2*pi/1000.0f)

bool debug_dynamicCameraDisableVibration=false;

DynamicCamera::DynamicCamera(GameObject* ref, GameField* field) :
  Camera(ref), currX(ref? ref->getX(): 0), currY(ref? ref->getY() : 0),
  currZ(1.0f), currT(0), vibration(0), vx(0), vy(0),
  zoom(0.35f), rotateMode(None), lookAhead(conf["conf"]["camera"]["lookahead"]), targetRotation(0),
  baseDetailLevel(conf["conf"]["graphics"]["detail_level"]), hud((Ship*)ref, field)
{
  const char* mode=conf["conf"]["camera"]["mode"];
  if      (0 == strcmp(mode, "none"))     rotateMode=None;
  else if (0 == strcmp(mode, "rotation")) rotateMode=Direction;
  else if (0 == strcmp(mode, "velocity")) rotateMode=Velocity;
}

DynamicCamera::~DynamicCamera() {
  asgi::reset();
  glClearColor(0,0,0,1);
  //Restore actual detail level config
  conf["conf"]["graphics"]["detail_level"]=(int)baseDetailLevel;
}

#define ref reference

void DynamicCamera::doSetup() noth {
  if (debug_dynamicCameraDisableVibration) vibration=0;
  if (!ref) return;

  //Adjust detail level for zoom. Always round down (for better
  //quality) and never decrease beyond the base
  conf["conf"]["graphics"]["detail_level"] = min(8, (int)(currZ > 1? baseDetailLevel
      : (unsigned)(baseDetailLevel/currZ)));

  //Handle vibration
  float vibx=vibration*(rand()/(float)RAND_MAX) - vibration/2,
        viby=vibration*(rand()/(float)RAND_MAX) - vibration/2;
  mTrans(0.5f, vheight/2-(rotateMode!=None? lookAhead*vheight : 0),
         matrix_stack::view);
  mUScale(currZ, matrix_stack::view);
  mRot(-currT, matrix_stack::view);
  mTrans(-currX+vibx, -currY+viby, matrix_stack::view);

  //We form a box around the largest possible area, regardless
  //of actual orientation.
  //A constantly-changing framerate is more distracting than
  //a consistently low framerate (eg, on slower computers)
  float cx=currX - vibx/currZ - (rotateMode!=None? lookAhead : 0)*sin(currT)/currZ,
        cy=currY - viby/currZ + (rotateMode!=None? lookAhead : 0)*cos(currT)/currZ;
  float ratio=sqrt(2.0f)/2.0f/currZ;
  cameraX1=cx - ratio;
  cameraX2=cx + ratio;
  cameraY1=cy - ratio/* *vheight*/;
  cameraY2=cy + ratio/* *vheight*/;
  cameraCX=currX - vibx/currZ;
  cameraCY=currY - viby/currZ;
  cameraZoom=currZ;

  float cornerAngle=atan2(vheight/2, 0.5f);
  float cornerDist=sqrt(vheight*vheight/4 + 0.25f)/currZ;
  screenCorners[0].first = cornerDist*cos(cornerAngle + currT) + cx;
  screenCorners[0].second= cornerDist*sin(cornerAngle + currT) + cy;
  screenCorners[1].first = cornerDist*cos(pi - cornerAngle + currT) + cx;
  screenCorners[1].second= cornerDist*sin(pi - cornerAngle + currT) + cy;
  screenCorners[2].first = cornerDist*cos(pi + cornerAngle + currT) + cx;
  screenCorners[2].second= cornerDist*sin(pi + cornerAngle + currT) + cy;
  screenCorners[3].first = cornerDist*cos(-cornerAngle + currT) + cx;
  screenCorners[3].second= cornerDist*sin(-cornerAngle + currT) + cy;

  //glLineWidth(currZ);
  //glPointSize(currZ);
  mId();
}

void DynamicCamera::update(float time) noth {
  Ship* s=NULL;
  hud.setRef(s=dynamic_cast<Ship*>(const_cast<GameObject*>(ref)));
  hud.update(time);

  targetRotation=getRotation();
  /* Lower frame-rates can give a "choppy" feel, since the math we use
   * will result in the camera jumping around a bit.
   * Force updates to a 10ms granularity.
   */
  for (float t=time; t>0; t -= 10) {
    float time = (t>10? 10 : t);

    currX+=vx*time;
    currY+=vy*time;

    if (currT!=targetRotation) {
      float diff=currT-targetRotation;
      if (diff<-pi) diff+=2*pi;
      if (diff>+pi) diff-=2*pi;

      if (diff<0) {
        if (fabs(diff)<ROT_SPEED*time) currT=targetRotation;
        else {
          currT+=ROT_SPEED*time;
          if (currT>2*pi) currT-=2*pi;
        }
      } else if (diff>0) {
        if (fabs(diff)<ROT_SPEED*time) currT=targetRotation;
        else {
          currT-=ROT_SPEED*time;
          if (currT<0) currT+=2*pi;
        }
      }
    }

    if (currZ<zoom) {
      currZ+=time*ZOOM_SPEED*currZ;
      if (currZ>zoom) currZ=zoom;
    } else if (currZ>zoom) {
      currZ-=time*ZOOM_SPEED*currZ;
      if (currZ<zoom) currZ=zoom;
    }

    vibration*=pow(STABILIZE_RATIO_SEC, time/1000.0f);

    if (!ref) continue;
    //Adjust vx and vy as necessary
    float tx=ref->getX(),
          ty=ref->getY();
    float dx=tx-currX, dy=ty-currY;
    currX+=dx*MOVE_SPEED*time;
    currY+=dy*MOVE_SPEED*time;

    float dvx=vx-ref->getVX(), dvy=vy-ref->getVY();
    vx-=dvx*ACCEL_SPEED*time;
    vy-=dvy*ACCEL_SPEED*time;
  }
}

void DynamicCamera::drawOverlays() noth {
  hud.draw(this);
}

void DynamicCamera::reset() noth {
  //currZ=zoom;
  currX=ref->getX();
  currY=ref->getY();
  vibration=0;
  vx=vy=0;
  currT=targetRotation=getRotation();
  float ratio=sqrt(2.0f)/currZ/2;
  cameraX1=currX-ratio;
  cameraX2=currX+ratio;
  cameraY1=currY-ratio*vheight;
  cameraY2=currY+ratio*vheight;
}

void DynamicCamera::impact(float amt) noth {
  vibration+=amt*15;
  hud.damage(amt);
}

float DynamicCamera::getZoom() const noth {
  return zoom;
}
void DynamicCamera::setZoom(float z) noth {
  if (z>=0.1f && z<=2.0f) zoom=z;
}

float DynamicCamera::getLookAhead() const noth {
  return lookAhead;
}
void DynamicCamera::setLookAhead(float l) noth {
  if (l>0 && l<vheight/2) lookAhead=l;
}

DynamicCamera::RotateMode DynamicCamera::getRotateMode() const noth {
  return rotateMode;
}
void DynamicCamera::setRotateMode(RotateMode mode) noth {
  rotateMode=mode;
}

//When in Velocity mode, when under this speed, we factor actual rotation in
//according to the closeness to this speed
#define MIN_VEL_SPEED 0.00005f
float DynamicCamera::getRotation() const noth {
  if (!ref) return 0;
  switch (rotateMode) {
    case None: return 0;
    case Direction: return ref->getRotation()-pi/2;
    case Velocity:
      float velang=atan2(ref->getVY(), ref->getVX());
      if (velang!=velang || sqrt(ref->getVX()*ref->getVX() + ref->getVY()*ref->getVY())<MIN_VEL_SPEED)
        return ref->getRotation()-pi/2;
      else
        return velang-pi/2;
  }
  return 0;
}

namespace dyncam_action {
  void zoom(Ship* s, ActionDatum& dat) {
    DynamicCamera* that=(DynamicCamera*)dat.pair.first;
    float amt=dat.pair.second.amt;
    that->setZoom(that->getZoom()+amt);
  }
  void lookahead(Ship* s, ActionDatum& dat) {
    DynamicCamera* that=(DynamicCamera*)dat.pair.first;
    float amt=dat.pair.second.amt;
    that->setLookAhead(that->getLookAhead()+amt);
    conf["conf"]["camera"]["lookahead"]=that->getLookAhead();
  }
  void set_mode(Ship* s, ActionDatum& dat) {
    DynamicCamera* that=(DynamicCamera*)dat.pair.first;
    const char* val=(const char*)dat.pair.second.ptr;
    if      (0 == strcmp(val, "none"    )) that->setRotateMode(DynamicCamera::None);
    else if (0 == strcmp(val, "rotation")) that->setRotateMode(DynamicCamera::Direction);
    else if (0 == strcmp(val, "velocity")) that->setRotateMode(DynamicCamera::Velocity);
    else return;

    conf["conf"]["camera"]["mode"]=val;
  }
};

void DynamicCamera::hc_conf_bind() {
  ActionDatum thisPair;
  thisPair.pair.first=(void*)this;
  thisPair.pair.second.ptr=NULL;

  DigitalAction da_zoom = {thisPair, true, false, dyncam_action::zoom, NULL},
                da_lkah = {thisPair, true, false, dyncam_action::lookahead, NULL},
                da_mode = {thisPair, false, false, dyncam_action::set_mode, NULL};
  bind( da_zoom, "adjust camera zoom", Float, true, getFloatLimit(5) );
  bind( da_lkah, "adjust camera lookahead", Float, true, getFloatLimit(0.5f) );
  bind( da_mode, "set camera mode", CString, true );
  hud.hc_conf_bind();
}
