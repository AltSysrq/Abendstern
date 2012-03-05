/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/cell.hxx
 */

#include <GL/gl.h>
#include <iostream>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <typeinfo>
#include <cstring>
#include <cassert>

#include "cell.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/sys/power_plant.hxx"
#include "src/ship/sys/engine.hxx"
#include "src/ship/sys/c/capacitor.hxx"
#include "src/ship/sys/a/dispersion_shield.hxx"
#include "src/globals.hxx"
#include "src/graphics/matops.hxx"
using namespace std;

float cell_colours::semiblack[4] = { 0,0,0,0.9f };
float cell_colours::white[4] = { 1,1,1,0.7f };

Cell::Cell(Ship* p) :
  netIndex(-1),
  parent(p), usage(CellSystems), damage(0),
  isSquare(false), isEmpty(false), oriented(false)
{
  if (p==NULL) return; //We were created for scratch
  for (int i=0; i<4; ++i) neighbours[i]=NULL;
  systems[0]=systems[1]=NULL;

  if (!headless) {
    glGenTextures(1, &damageTexture);
    memset(damageTextureData, 0, sizeof(damageTextureData));
    glBindTexture(GL_TEXTURE_2D, damageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, damageTextureData);
    glBindTexture(GL_TEXTURE_2D, 0);
  } else damageTexture = 0;

  //Nothing is larger than a square
  collisionRectangle.radius=STD_CELL_SZ*sqrt(2.0f);
  collisionRectangle.data = this;

  physics.valid = 0;
  physics.nearestDS = physics.nextDepDS = physics.torquePair = NULL;
}

void Cell::orient(int initTheta) throw (range_error) {
  //We are root
  x=y=0;
  theta=initTheta;
  oriented=true;
  orientImpl();
}
void Cell::orientImpl() throw (range_error) {
  for (int i=0; i<4; ++i)
    if (neighbours[i] && !neighbours[i]->oriented) {
      neighbours[i]->oriented=true;
      int index=neighbours[i]->getNeighbour(this);
      neighbours[i]->theta = (this->theta +
                              //this->edgeDT(i) +
                              (180 -
                               (neighbours[i]->edgeT(index)-this->edgeT(i)))
                              ) % 360;
      if (neighbours[i]->theta < 0) neighbours[i]->theta+=360;
      float angle = ((this->theta + this->edgeT(i)) % 360)*2.0f*pi/360;
      float dx=this->edgeD(i)+neighbours[i]->edgeD(index);
      neighbours[i]->x = this->x + dx*cos(angle);
      neighbours[i]->y = this->y + dx*sin(angle);
      neighbours[i]->orientImpl();
    }
}

unsigned Cell::getNeighbour(const Cell* that) throw (range_error) {
  for (int i=0; i<4; ++i) if (neighbours[i]==that) return i;
  throw range_error("The given cell is not one of my neighbours!");
}

void Cell::draw(bool translate) noth {
  BEGINGP("Cell")
  if (translate) {
    mPush();
    mTrans(x, y);
    mRot(theta*pi/180);
  }
  drawThis();
  if (translate)
    mPop();
  ENDGP
}

void Cell::drawDamage() noth {
  //Don't bother drawing damage if there is none to display
  if (damage == 0 || !damageTexture) return;

  mPush();
  mTrans(x,y);
  mRot(theta*pi/180);
  glBindTexture(GL_TEXTURE_2D, damageTexture);
  drawDamageThis();
  glBindTexture(GL_TEXTURE_2D, 0);
  mPop();
}

void Cell::drawShape(const float* healthy, const float* damaged) noth {
  BEGINGP("Cell::drawShape")
  mPush();
  mTrans(x, y);
  mRot(theta);
  float max=getMaxDamage();
  //Report lower if we have a power generator, since they can explode
  for (int i=0; i<2; ++i) if (systems[i]) {
    PowerPlant* test=NULL;
    test=dynamic_cast<PowerPlant*>(systems[i]);
    if (test) max=getMaxDamage()-getIntrinsicDamage();
  }
  if (max==0) max=0.001;
  float dmgpct=damage/max;
  //Make damage more apparent
  dmgpct*=1.2f;
  if (dmgpct>1) dmgpct=1;
  drawShapeThis(healthy[0]*(1-dmgpct) + damaged[0]*dmgpct,
                healthy[1]*(1-dmgpct) + damaged[1]*dmgpct,
                healthy[2]*(1-dmgpct) + damaged[2]*dmgpct,
                healthy[3]*(1-dmgpct) + damaged[3]*dmgpct);
  mPop();
  ENDGP
}

void Cell::getAdjoined(vector<Cell*>& list) noth {
  for (unsigned int i=0; i<list.size(); ++i)
    if (list[i]==this) return;

  list.push_back(this);
  unsigned n = numNeighbours();
  for (unsigned i=0; i<n; ++i)
    if (neighbours[i])
      neighbours[i]->getAdjoined(list);
}

float Cell::getMaxDamage() const noth {
  const_cast<Cell*>(this)->physicsRequire(PHYS_CELL_REINFORCEMENT_BIT);
  return getIntrinsicDamage()*(1+parent->getReinforcement())*
         physics.reinforcement;
}

float Cell::getCurrDamage() const noth {
  return damage;
}

bool Cell::applyDamage(float amount, unsigned blame)
noth {
  //catch bug
  if (amount!=amount) {
    cout << "Cell::applyDamage(float): amount is NaN! Comitting suicide..." << endl;
    ++*((int*)NULL);
  }

  float maxDamage=getMaxDamage();

  //Each "damage point" as we'll call it will add 16 to the alpha of a
  //random pixel in damageTextureData that is not already 240
  //That means there are 16*16*(256/16)*224/256 (for 224 avg)=3584 exactly possible points
  #define TOTAL_POINTS (16*16*16*224/256)
  int currentPoints=(int)(damage/maxDamage*TOTAL_POINTS);
  damage+=amount;
  //Don't bother drawing if too great
  if (!headless && damage<maxDamage) {
    parent->cellDamaged(this);
    int newCurrentPoints=(int)(damage/maxDamage*TOTAL_POINTS);
    int newPoints=newCurrentPoints-currentPoints;
    if (newPoints) {
      while (newPoints) {
        int x=rand()&15, y=rand()&15;
        if (damageTextureData[y*16*4+x*4 + 3]<240) {
          damageTextureData[y*16*4+x*4 + 3]+=16;
          --newPoints;
        }
      }

      //Refresh texture data when needed
      if (damageTexture==0) {
        glGenTextures(1, &damageTexture);
      }

      glBindTexture(GL_TEXTURE_2D, damageTexture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, damageTextureData);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  if (!parent->isRemote && damage>maxDamage-getIntrinsicDamage() &&
      damage<maxDamage) {
    bool destruction=false;
    if (systems[0]) if (!systems[0]->damage(blame)) {
      delete systems[0];
      systems[0]=NULL;
      destruction=true;
    }
    if (systems[1]) if (!systems[1]->damage(blame)) {
      delete systems[1];
      systems[1]=NULL;
      destruction=true;
    }
    if (destruction) {
      //Assumes only power generators and miscelaneous systems can explode
      physics_bits bits = PHYS_CELL_POWER_BITS | PHYS_CELL_MASS_BITS
                        | PHYS_CELL_POWER_PROD_BITS;
      physicsClear(bits);
      parent->cellChanged(this);
      parent->refreshUpdates();
    }
  } else if (!parent->isRemote && damage >= maxDamage) {
    if (systems[0]) systems[0]->destroy(blame);
    if (systems[1]) systems[1]->destroy(blame);
  }

  return damage<maxDamage;
}

void Cell::physicsRequire(physics_bits bits) noth {
  //We can do nothing about ship-global bits
  bits &= PHYS_CELL_MASK;

  //If empty, all the constructor-set values are correct, except possibly
  //location properties
  if (isEmpty) {
    physics.valid |= PHYS_CELL_ALL & ~PHYS_CELL_LOCATION_PROPERTIES_BIT;
    return;
  }

  //Return early if already satisfied
  if (bits == (bits & physics.valid)) return;

  /* Each check has the following two conditions:
   *   !(physics.valid & SOME_BIT)      Determine whether we know SOME value
   *   (bits & SOME_BITS)               Determine whether we need to know it
   */

  if (!(physics.valid & PHYS_CELL_MASS_BIT) && (PHYS_CELL_MASS_BITS & bits)) {
    physics.mass = intrinsicMass();
    if (usage == CellBridge) physics.mass += 10;
    float reinMul = (1 + parent->getReinforcement());
    physics.mass *= reinMul * reinMul;
    if (systems[0]) physics.mass += systems[0]->mass();
    if (systems[1]) physics.mass += systems[1]->mass();

    physics.valid |= PHYS_CELL_MASS_BIT;
  }

  if (!(physics.valid & PHYS_CELL_LOCATION_PROPERTIES_BIT)
  &&  (PHYS_CELL_LOCATION_PROPERTIES_BITS & bits)) {
    parent->physicsRequire(PHYS_SHIP_COORDS_BIT);

    physics.distance = sqrt(x*x + y*y);
    if (physics.distance > 0) {
      physics.cosine = x/physics.distance;
      physics.sine = y/physics.distance;
      physics.angle = atan2(y,x);
    } else {
      physics.cosine = 1;
      physics.sine = 0;
      physics.angle = 0;
    }

    physics.valid |= PHYS_CELL_LOCATION_PROPERTIES_BIT;
  }

  if (!(physics.valid & PHYS_CELL_THRUST_BIT) && (PHYS_CELL_THRUST_BITS & bits)) {
    physics.thrustX = physics.thrustY = 0;
    if (systems[0] && systems[0]->clazz == Classification_Engine
    &&  (!parent->isStealth() || systems[0]->supportsStealthMode())) {
      Engine* e = (Engine*)systems[0];
      physics.thrustX -= e->thrust() * cos(e->getThrustAngle());
      physics.thrustY -= e->thrust() * sin(e->getThrustAngle());
    }
    else //There can only be one engine
    if (systems[1] && systems[1]->clazz == Classification_Engine
    &&  (!parent->isStealth() || systems[1]->supportsStealthMode())) {
      Engine* e = (Engine*)systems[1];
      physics.thrustX -= e->thrust() * cos(e->getThrustAngle());
      physics.thrustY -= e->thrust() * sin(e->getThrustAngle());
    }

    physics.valid |= PHYS_CELL_THRUST_BIT;
  }

  if (!(physics.valid & PHYS_CELL_TORQUE_BIT) && (PHYS_CELL_TORQUE_BITS & bits)) {
    //This does NOT take into account the equal-balance pairing
    //(That's up to the Ship with PHYS_SHIP_TORQUE_BIT)
    physics.torque = (physics.thrustX*y - physics.thrustY*x);

    physics.valid |= PHYS_CELL_TORQUE_BIT;
  }

  if (!(physics.valid & PHYS_CELL_ROT_THRUST_BIT) && (PHYS_CELL_ROT_THRUST_BITS & bits)) {
    physics.rotationalThrust = fabs(physics.torque);

    if (systems[0] && systems[0]->clazz == Classification_Engine)
      physics.rotationalThrust *= ((Engine*)systems[0])->getRotationalMultiplier();
    else if (systems[1] && systems[1]->clazz == Classification_Engine)
      physics.rotationalThrust *= ((Engine*)systems[1])->getRotationalMultiplier();

    physics.valid |= PHYS_CELL_ROT_THRUST_BIT;
  }

  if (!(physics.valid & PHYS_CELL_COOLING_BIT) && (PHYS_CELL_COOLING_BITS & bits)) {
    physics.cooling = 0;
    if (systems[0])
      physics.cooling += systems[0]->cooling_amount();
    if (systems[1])
      physics.cooling += systems[1]->cooling_amount();

    physics.valid |= PHYS_CELL_COOLING_BIT;
  }

  if (!(physics.valid & PHYS_CELL_HEAT_BIT) && (PHYS_CELL_HEAT_BITS & bits)) {
    physics.numHeaters = 0;
    if (systems[0]) physics.numHeaters += systems[0]->heating_count();
    if (systems[1]) physics.numHeaters += systems[1]->heating_count();

    physics.valid |= PHYS_CELL_HEAT_BIT;
  }

  //It is trivial (and probably faster) to handle both power bits
  //in the same code
  if ((!(physics.valid & PHYS_CELL_POWER_BIT)      && (PHYS_CELL_POWER_BITS & bits))
  ||  (!(physics.valid & PHYS_CELL_POWER_PROD_BIT) && (PHYS_CELL_POWER_PROD_BITS & bits))) {
    signed base = (usage == CellBridge? 3 : 1);
    physics.powerSC = physics.powerUC = base;
    physics.powerST = physics.powerUT = 0;
    physics.ppowerU = physics.ppowerS = 0;
    for (unsigned i=0; i<2; ++i) if (systems[i]) {
      signed u = systems[i]->normalPowerUse();
      signed s = systems[i]->stealthPowerUse();
      if (systems[i]->clazz == Classification_Engine) {
        //Add to throttled
        physics.powerST += s;
        physics.powerUT += u;
      } else {
        //Add to continuous
        physics.powerSC += s;
        physics.powerUC += u;
        //Might be generator
        if (s < 0) physics.ppowerS -= s;
        if (u < 0) physics.ppowerU -= u;
      }
    }

    physics.valid |= PHYS_CELL_POWER_PROD_BIT;
    physics.valid |= PHYS_CELL_POWER_BIT;
  }

  if (!(physics.valid & PHYS_CELL_CAPAC_BIT) && (PHYS_CELL_CAPAC_BITS & bits)) {
    physics.capacitance = 0;
    if (systems[0] && typeid(*systems[0]) == typeid(Capacitor))
      physics.capacitance += ((Capacitor*)systems[0])->getCapacity();
    if (systems[1] && typeid(*systems[1]) == typeid(Capacitor))
      physics.capacitance += ((Capacitor*)systems[1])->getCapacity();
    physics.capacitance *= 1000;

    physics.valid |= PHYS_CELL_CAPAC_BIT;
  }

  if (!(physics.valid & PHYS_CELL_REINFORCEMENT_BIT) && (PHYS_CELL_REINFORCEMENT_BITS & bits)) {
    physics.reinforcement = 1;
    if (systems[0] && (!parent->isStealth() || systems[0]->supportsStealthMode()))
      physics.reinforcement *= systems[0]->reinforcement_getAmt();
    if (systems[1] && (!parent->isStealth() || systems[1]->supportsStealthMode()))
      physics.reinforcement *= systems[1]->reinforcement_getAmt();

    physics.valid |= PHYS_CELL_REINFORCEMENT_BIT;
  }

  if (!(physics.valid & PHYS_CELL_DS_EXIST_BIT) && (PHYS_CELL_DS_EXIST_BITS & bits)) {
    physics.hasDispersionShield = (
        (systems[0] && typeid(*systems[0]) == typeid(DispersionShield))
     || (systems[1] && typeid(*systems[1]) == typeid(DispersionShield)));

    //We need to make sure the linked-list is valid
    if (physics.hasDispersionShield) {
      physics.distanceDS = 0;
      physics.nearestDS = this;
      physics.nextDepDS = NULL;
      //This happens to validate PHYS_CELL_DS_NEAREST_BIT
      physics.valid |= PHYS_CELL_DS_NEAREST_BIT;
    }

    physics.valid |= PHYS_CELL_DS_EXIST_BIT;
  }

  if (!(physics.valid & PHYS_CELL_DS_NEAREST_BIT) && (PHYS_CELL_DS_NEAREST_BITS & bits)) {
    float dist;
    Cell* nearest = parent->nearestDispersionShield(this, dist);
    if (nearest) {
      physics.distanceDS = dist;
      physics.nearestDS = nearest;
      //Add this to the head of the list
      physics.nextDepDS = nearest->physics.nextDepDS;
      nearest->physics.nextDepDS = this;
    } else {
      physics.nearestDS = NULL;
      physics.nextDepDS = NULL;
      physics.distanceDS = -1;
    }

    physics.valid |= PHYS_CELL_DS_NEAREST_BIT;
  }

  //Ensure we covered everything
  assert((physics.valid & bits) == bits);
}

void Cell::clearDSChain() noth {
  if (!(physics.valid & PHYS_CELL_DS_EXIST_BIT)) return;
  Cell* curr = physics.nextDepDS, *nxt;
  while (curr) {
    nxt = curr->physics.nextDepDS;
    curr->physics.nearestDS = NULL;
    curr->physics.distanceDS = -1;
    curr->physics.nextDepDS = NULL;
    curr->physicsClear(PHYS_CELL_DS_NEAREST_BITS);
    curr = nxt;
  }
}

void Cell::freeTexture() noth {
  if (!damageTexture) return;
  glDeleteTextures(1, &damageTexture);
  damageTexture=0;
}

Cell::~Cell() {
  if (systems[0]) delete systems[0];
  if (systems[1]) delete systems[1];
  if (!headless && damageTexture) glDeleteTextures(1, &damageTexture);
}
