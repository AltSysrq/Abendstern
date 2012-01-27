/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/launcher.hxx
 */

#include <cmath>
#include <cstring>

#include "launcher.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/core/lxn.hxx"
using namespace std;

Launcher::Launcher(Ship* parent, GLuint tex, Weapon _className, int min, int max)
: ShipSystem(parent, tex, Classification_Weapon, ShipSystem::Forward, ShipSystem::Small, _className),
  className(_className), energyLevel(3),
  readyAt(0), minPower(min), maxPower(max)
{ }

const char* Launcher::autoOrient() noth {
  Cell* cell=container;
  //Take the exposed side with the least difference from 0
  bool found=false;
  int minDelta=360;
  for (unsigned i=0; i<cell->numNeighbours(); ++i) {
    if (cell->neighbours[i]) continue;
    found=true;
    float sth=((int)(cell->getT()+cell->edgeT(i)))%360;
    if (sth>180) sth=sth-360;
    if (fabs(sth)<minDelta) {
      orientation=i;
      minDelta=(int)fabs(sth);
      theta=sth*pi/180;
      autoRotate=theta - cell->getT()*pi/180;
    }
  }

  if (found) return NULL;
  else RETL10N(launcher,no_exposed_faces)
}

const char* Launcher::setOrientation(int i) noth {
  if (i==-1) return autoOrient();
  if (i < -1 || i > 3) RETL10N(launcher,invalid_orientation)

  Cell* cell=container;
  if (container->neighbours[i]) RETL10N(launcher,selected_face_occupied)
  orientation=i;
  float sth=((int)(cell->getT()+cell->edgeT(i)))%360;
  theta=sth*pi/180;
  autoRotate=theta - cell->getT()*pi/180;
  return NULL;
}

int Launcher::getOrientation() const noth {
  return orientation;
}

float Launcher::getLaunchAngle() const noth {
  return theta;
}

//Match or return
#define MOR if (clazz!=className) return
void Launcher::weapon_enumerate(Weapon clazz, vector<ShipSystem*>& v) noth {
  MOR;
  v.push_back(this);
}

void Launcher::weapon_fire(Weapon clazz)  noth{
  MOR;
  if (!weapon_isReady(className)) return;
  if (parent->drawPower(getFirePower())) {
    readyAt=parent->getField()->fieldClock + (Uint32)getArmTime();
    GameObject* go=createProjectile();
    if (go)
      parent->getField()->addBegin(go);
  }
}

int Launcher::weapon_getEnergyLevel(Weapon clazz) const noth {
  MOR -1;
  return energyLevel;
}

int Launcher::weapon_getMinEnergyLevel(Weapon clazz) const  noth{
  MOR -1;
  return minPower;
}

int Launcher::weapon_getMaxEnergyLevel(Weapon clazz) const noth {
  MOR -1;
  return maxPower;
}

void Launcher::weapon_setEnergyLevel(Weapon clazz, int val)  noth{
  MOR;
  if (val < minPower)
    energyLevel=minPower;
  else if (val > maxPower)
    energyLevel=maxPower;
  else
    energyLevel=val;
}

bool Launcher::weapon_isReady(Weapon clazz) const noth {
  MOR false;
  return !parent->isStealth() && parent->getField()->fieldClock >= readyAt;
}

float Launcher::weapon_getLaunchEnergy(Weapon clazz) const noth {
  MOR ShipSystem::weapon_getLaunchEnergy(clazz);
  return getFirePower();
}
