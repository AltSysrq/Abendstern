/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/ship.hxx
 */

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <cfloat>
#include <typeinfo>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <exception>
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <cassert>

#include <GL/gl.h>

#include "src/sim/game_field.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"
#include "src/sim/blast.hxx"
#include "src/fasttrig.hxx"
#include "ship.hxx"
#ifndef AB_OPENGL_14
#include "ship_renderer.hxx"
#endif
#include "cell/cell.hxx"
#include "cell/empty_cell.hxx"
#include "cell/circle_cell.hxx"
#include "cell/equt_cell.hxx"
#include "cell/rightt_cell.hxx"
#include "cell/square_cell.hxx"
#include "sys/power_plant.hxx"
#include "sys/engine.hxx"
#include "sys/c/capacitor.hxx"
#include "auxobj/plasma_fire.hxx"
#include "auxobj/shield.hxx"
#include "auxobj/cell_fragment.hxx"
#include "sys/b/plasma_burst_launcher.hxx"
#include "src/weapon/particle_beam.hxx"
#include "src/camera/forwarding_effects_handler.hxx"
#include "src/graphics/matops.hxx"
#include "src/audio/ship_mixer.hxx"
#include "src/audio/ship_effects.hxx"
#include "src/background/nebula.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"
#include "src/control/human_controller.hxx"
#include "src/net/ship_damage_geraet.hxx"

using namespace std;

#ifdef AB_OPENGL_14
struct ShipRenderer {
  int a;
  void cellRemoved(Cell*) {}
  void cellDamage(Cell*) {}
  void cellChanged(Cell*) {}
};
#endif /* AB_OPENGL_14 */

//10 minutes max invisible fragment time
#define MAX_INVFRAG_TIME (10.0f*60.0f*1000)

//Multiply damage absorption by this amount to find
//the temperature change for dispersion shielding
#define DAM_DISP_TEMP_MUL 5.0f

//Multiply damage absorption by this amount to find
//the capacitance requirement for dispersion shielding
#define DAM_DISP_CAP_MUL 15000.0f

//The maximum damage that dispersion shielding can
//absorb, when the distance is zero cells
#define DAM_DISP_MAX 2048.0f

//The multiplier for distance when determining effectiveness
//of the dispersion shield.
#define DAM_DISP_DIST_MUL 1.2f

//The per-cell power requirements of being cloaked
#define CLOAK_POWER_REQ 2.5f

//In order to force a manoeuverability disadvntage onto
//larger ships, multiply inertia by this factor
#define INERTIA_FUDGE_FACTOR 9.0f

//Volume multiplier for impact sounds
#define IMPACT_VOLUME_MUL 0.07f

#define SPIN_UP 8192
#define SPIN_DOWN 500.0f

static inline float randomf() noth {
  return (rand()%RAND_MAX)/(float)RAND_MAX;
}

Ship::Ship(GameField* field) :
  GameObject(field, randomf()*field->width, randomf()*field->height),
  controller(NULL), effects(new ForwardingEffectsHandler(this)),
  renderer(NULL),
  validPhysics(0),
  colourR(0.8f), colourG(0.8f), colourB(0.8f),
  theta(rand()*2*pi/(float)RAND_MAX), vtheta(0),
  cosTheta(cos(theta)), sinTheta(sin(theta)),
  isFragment(false),
  currentCapacitance(0),
  thrustPercent(1),
  thrustOn(false), brakeOn(false),
  reinforcement(0),
  powerTest(false),
  invisibleFragTime(0),
  invisibleTime(0),
  shieldsDeactivated(false),
  stealthMode(false), stealthCounter(0),
  hasCloakingDevice(false),
  timeUntilSlowFire(0),
  radar(&defaultRadar), timeUntilRadarRefresh(0),
  soundEffects(false),
  blame(0xFFFFFF),
  score(0),
  diedSpontaneously(false),
  damageMultiplier(1.0f),
  shipExistenceFailure(NULL),
  shipDamageGeraet(NULL)
{
  insignia=reinterpret_cast<unsigned long>(this);
  classification = GameObject::ClassShip;
  isExportable=true;
  isTransient=false;
  nebulaInteraction=true;
  memset(damageBlame, 0, sizeof(damageBlame));
}

Ship::Ship(const Ship& other)
: GameObject(other.field, other.x, other.y, other.vx, other.vy),
  controller(NULL), effects(new ForwardingEffectsHandler(this)),
  target(other.target), typeName(other.typeName),
  engineInfo(other.engineInfo),
  plasmaBurstLauncherInfo(other.plasmaBurstLauncherInfo),
  heatInfo(other.heatInfo),
  renderer(NULL),
  validPhysics(0),
  colourR(other.colourR), colourG(other.colourG), colourB(other.colourB),
  theta(other.theta), vtheta(other.vtheta),
  cosTheta(other.cosTheta), sinTheta(other.sinTheta),
  isFragment(other.isFragment),
  currentCapacitance(other.currentCapacitance),
  thrustPercent(other.thrustPercent),
  thrustOn(other.thrustOn), brakeOn(other.brakeOn),
  reinforcement(other.reinforcement),
  powerTest(false),
  invisibleFragTime(other.invisibleFragTime),
  invisibleTime(other.invisibleTime),
  shieldsDeactivated(other.shieldsDeactivated),
  stealthMode(other.stealthMode), stealthCounter(other.stealthCounter),
  hasCloakingDevice(other.hasCloakingDevice),
  timeUntilSlowFire(other.timeUntilSlowFire),
  timeUntilRadarRefresh(other.timeUntilRadarRefresh),
  soundEffects(false),
  insignia(other.insignia),
  blame(other.blame),
  score(other.score),
  diedSpontaneously(other.diedSpontaneously),
  damageMultiplier(other.damageMultiplier),
  shipExistenceFailure(other.shipExistenceFailure),
  shipDamageGeraet(NULL)
{
  classification = GameObject::ClassShip;
  isExportable=true;
  isTransient = false;
  nebulaInteraction=true;
  memset(damageBlame, 0, sizeof(damageBlame));

  setRadar(other.getRadar());

  /* To copy the cells:
   * 0. Create an instance of the same class of each Cell in the original
   * 1. Use the indices of the neighbours in the original to replicate
   *    the neighbour structure in ourself
   * 2. Orient our new Cells
   * 3. Clone the systems from the original to ourself
   * 4. Detect physics without compensation
   */
  for (unsigned i=0; i<other.cells.size(); ++i) {
    const type_info& type=typeid(*other.cells[i]);
    #define TYP(typ) if (type==typeid(typ)) cells.push_back(new typ(this)); else
    TYP(SquareCell)
    TYP(CircleCell)
    TYP(EquTCell)
    TYP(RightTCell)
    TYP(EmptyCell) {
      cerr << "FATAL: Forgot cell type " << type.name() << endl;
      exit(EXIT_PROGRAM_BUG);
    }
    #undef TYP
  }
  for (unsigned i=0; i<cells.size(); ++i) for (int n=0; n<4; ++n) if (other.cells[i]->neighbours[n]) {
    unsigned index=find(other.cells.begin(), other.cells.end(), other.cells[i]->neighbours[n])
                  - other.cells.begin();
    cells[i]->neighbours[n]=cells[index];
  }
  cells[0]->orient();

  for (unsigned i=0; i<cells.size(); ++i) for (int s=0; s<2; ++s) if (other.cells[i]->systems[s]) {
    cells[i]->systems[s]=other.cells[i]->systems[s]->clone();
  }

  refreshUpdates();
}

Ship::~Ship() {
  if (controller) delete controller;
  for (unsigned int i=0; i<cells.size(); ++i) delete cells[i];
  radar->delClient(this);
  radar->superPurge(this);
  if (renderer) delete renderer;
  if (shipDamageGeraet)
    shipDamageGeraet->delRemoteShip(this);

  disableSoundEffects();
}

float* Ship::temporaryZero() noth {
  float* f=new float[5];
  f[0]=x;
  f[1]=y;
  f[2]=theta;
  f[3]=cosTheta;
  f[4]=sinTheta;

  x=y=0;
  theta=0;
  cosTheta=1;
  sinTheta=0;

  return f;
}

void Ship::restoreFromZero(float* f) noth {
  x=f[0];
  y=f[1];
  theta=f[2];
  cosTheta=f[3];
  sinTheta=f[4];
  delete[] f;
}

bool Ship::update(float et) noth {
  if (soundEffects && currentVFrameLast)
    audio::shipSoundEffects(currentFrameTime,this);

  physicsRequire(PHYS_SHIP_MASS_BIT
                |PHYS_SHIP_INERTIA_BIT
                |PHYS_SHIP_POWER_BIT
                |PHYS_SHIP_THRUST_BIT
                |PHYS_SHIP_TORQUE_BIT
                |PHYS_SHIP_ROT_THRUST_BIT
                |PHYS_SHIP_CAPAC_BIT
                |PHYS_SHIP_COOLING_BIT
                |PHYS_SHIP_ENGINE_INVENTORY_BIT
                |PHYS_SHIP_SHIELD_INVENTORY_BIT);

  //The controller only need be updated per physical frame
  if (controller && currentVFrameLast) controller->update(currentFrameTime);

  x+=vx*et;
  y+=vy*et;
  theta+=vtheta*et;
  if      (theta<0)    theta=2*pi-fmod((-theta),(2*pi));
  else if (theta>2*pi) theta=fmod(theta, (2*pi));
  cosTheta = cos(theta);
  sinTheta = sin(theta);
  vtheta += nebulaTorsion/angularInertia * et;
  vx += nebulaFrictionX/mass * et;
  vy += nebulaFrictionY/mass * et;
  timeUntilSlowFire -= et;

  collisionTree.reset(x, y, cosTheta, sinTheta);

  if (isRemote) {
    REMOTE_XYCK;
  }

  if (invisibleTime<5000) {
    invisibleTime+=et;
    if (invisibleTime>=5000) {
      if (renderer) delete renderer;
      renderer=NULL;
      for (unsigned i=0; i<cells.size(); ++i)
        cells[i]->freeTexture();
    }
  }

  if (currentCapacitance > totalCapacitance) currentCapacitance = totalCapacitance;
  if (!isFragment) {
    //Only evaluate power if local
    if (!isRemote) {
      //Allow using more power than is available
      retryPowerSupply:
      physicsRequire(PHYS_SHIP_POWER_BIT);
      if (currPowerDrain>currPowerProd) {
        if (currentCapacitance>0) {
          currentCapacitance-=(currPowerDrain - currPowerProd)*et;
          if (currentCapacitance<0) currentCapacitance=0;
        //Can not support any longer, cut engines
        } else if (thrustOn) {
          setThrustOn(false);
          goto retryPowerSupply;
        //Cut brake as well if we must
        } else if (brakeOn) {
          setBrakeOn(false);
          goto retryPowerSupply;
        //Disable cloaking device
        } else if (hasCloakingDevice) {
          hasCloakingDevice = false;
          updateCurrPower();
          goto retryPowerSupply;
        //Exit stealth mode
        } else if (stealthMode) {
          setStealthMode(false);
          goto retryPowerSupply;
        //Kill the shields
        } else if (!shieldsDeactivated) {
          shield_deactivate(this);
          shieldsDeactivated=true;
          //Clear all cell power bits that have shields
          physicsRequire(PHYS_SHIP_SHIELD_INVENTORY_BIT);
          for (unsigned i=0; i<shields.size(); ++i)
            shields[i]->getParent()->physicsClear(PHYS_CELL_POWER_BITS | PHYS_CELL_POWER_PROD_BITS);
          shields.clear();
          physicsClear(PHYS_SHIP_POWER_BITS);
          physicsRequire(PHYS_SHIP_POWER_BIT);
          goto retryPowerSupply;
        //The ship can no longer run.
        } else {
          //The ship becomes a drifting fragment
          //and simply floats around from hereonout
          isFragment=decorative=true;
          death(false);
          currentCapacitance=0;
        }
      } else {
        if (currentCapacitance<totalCapacitance) {
          currentCapacitance+=(currPowerProd-currPowerDrain)*et/2;
          if (currentCapacitance>totalCapacitance) currentCapacitance=totalCapacitance;
        }
      }
    }

    if (rotationalThrust>0 && !isRemote)
      vtheta*=(SPIN_DOWN-et)/SPIN_DOWN;

    if (isFragment) thrustOn=brakeOn=false;
    if (thrustOn) accel(et);
    if (brakeOn ) decel(et);

    //Update all systems
    //Copy the vector, as the contents may change during the calls
    const vector<SystemUpdate> cellUpdateFunctions(this->cellUpdateFunctions);
    unsigned size=cellUpdateFunctions.size();
    for (unsigned int i=0; i<size; ++i) {
      cellUpdateFunctions[i](et);
    }

    if (stealthMode && stealthCounter < STEALTH_COUNTER_MAX)
      stealthCounter = min(STEALTH_COUNTER_MAX, (int)(stealthCounter+et));
    else if (!stealthMode && stealthCounter > STEALTH_COUNTER_MIN)
      stealthCounter = max(STEALTH_COUNTER_MIN, (int)(stealthCounter-et));

    //Handle radar
    if ((timeUntilRadarRefresh -= et) < 0) {
      timeUntilRadarRefresh = rand()/(float)RAND_MAX*500.0f+500.0f;
      radar->clear(this);

      unsigned size=field->size();
      for (unsigned i=0; i<size; ++i) {
        GameObject* obj=field->at(i);
        if (obj->getClassification() == GameObject::ClassShip) {
          Ship* s=(Ship*)obj;
          if (s->hasPower()) {
            //Determine whether the distance is OK
            float maxDist = s->getRadius() * (s->isStealth()? s->isCloaked()? CLOAK_VISIBILITY
                                              : STEALTH_VISIBILITY : UNSTEALTH_VISIBILITY);
            float dx = x-s->getX();
            float dy = y-s->getY();
            if (field->perfectRadar || dx*dx + dy*dy < maxDist*maxDist)
              radar->insert(s, this);
          }
        }
      }
    }
  } else if (!isRemote) {
    //Just use a fast method to determine visibility
    if (x+radius<cameraX1 || x-radius>cameraX2 ||
        y+radius<cameraY1 || y-radius>cameraY2) invisibleFragTime+=et;
    else invisibleFragTime=0;

    if (invisibleFragTime>MAX_INVFRAG_TIME && !isRemote) {
      death(true);
      return false;
    }
  }

  //Update shields
  for (unsigned i=0; i<shields.size(); ++i)
    shields[i]->update(et);

  //Update systems and damage if last frame
  if (currentVFrameLast && !isFragment) {
    Engine::update(this, currentFrameTime);

    //Increase by a factor of 0.2/sec, limit 3
    heatInfo.coolRate += et/5000.0f;
    if (heatInfo.coolRate > 3)
      heatInfo.coolRate=3;
    //Drop coolRate*10 degrees/sec
    if (heatInfo.temperature > ROOM_TEMPERATURE)
      heatInfo.temperature -=
          heatInfo.coolRate*currentFrameTime/100*(1+getCoolingMult());

    float dmgmul = pow(0.5f, et/10000.0f);
    for (unsigned i=0; i<lenof(damageBlame); ++i)
      damageBlame[i].damage *= dmgmul;
  }

  if (this->x != this->x) {
    cerr << "x is NaN!" << endl;
    assert(false);
    exit(EXIT_THE_SKY_IS_FALLING);
  }

  return true;
}

void Ship::draw() noth {
  physicsRequire(PHYS_SHIP_CAPAC_BIT
                |PHYS_SHIP_POWER_BIT
                |PHYS_SHIP_SHIELD_INVENTORY_BIT
                |PHYS_SHIP_COORDS_BIT
                |PHYS_CELL_LOCATION_PROPERTIES_BIT
                );

  BEGINGP("Ship")
  invisibleTime=0;
  mPush();
  mTrans(x, y);
  mRot(theta);

#ifndef AB_OPENGL_14
  if (!renderer)
    renderer = new ShipRenderer(this);

  renderer->setPallet(P_CAPACITOR, getCapacitancePercent(), getCapacitancePercent(), getCapacitancePercent());
  unsigned pulse=field->fieldClock%1000;
  if (pulse<500) pulse=1000-pulse;
  if (hasPower() && !isStealth())
    renderer->setPallet(P_FISSION_POW, getPowerUsagePercent(), (pulse/1000.0f)*(pulse/1000.0f), 0);
  else
    renderer->setPallet(P_FISSION_POW, 0.2, 0.35, 0.2);
  unsigned pulse2 = ((unsigned)(field->fieldClock*getPowerUsagePercent()))%400;
  if (pulse2<200) pulse2=400-pulse2;
  if (hasPower() && !isStealth())
    renderer->setPallet(P_FUSION_POW, 0.5f+pulse2/400.0f*0.5f, pulse2/400.0f, 0);
  else
    renderer->setPallet(P_FUSION_POW, 0.4, 0.3, 0.2);
  if (hasPower() && !isStealth())
    renderer->setPallet(P_ANTIM_POW, (pulse2/400.0f)*(pulse/1000.0f), pulse2/400.0f*0.75f+0.25f, 1);
  else
    renderer->setPallet(P_ANTIM_POW, 0.1, 0.2, 0.4);

  renderer->setPallet(P_HEATSINK, heatInfo.temperature/10000.0f, 0.2f, 0.9f);

  unsigned shieldLoc = (isFragment || stealthMode? 0 : field->fieldClock/120 % 6);
  float f=0;
  while (f < 1.01) {
    renderer->setPallet(P_SHIELD0+shieldLoc, f, f, f);
    f += 0.2f;
    ++shieldLoc;
    shieldLoc %= 6;
  }

  unsigned bramjLoc = field->fieldClock/100 % 4;
  for (unsigned i=P_BUSSARD0; i<=P_BUSSARD3; ++i)
    if (i == bramjLoc+P_BUSSARD0)
      renderer->setPallet(i, 0.6f+0.4f*engineInfo.fade, engineInfo.fade/2, 0);
    else
      renderer->setPallet(i, 0.6f, 0, 0);

  renderer->draw();
#else /* defined(AB_OPENGL_14) */
  //Don't draw if in stealth mode and have cloaking device
  //Exception: If being controlled by the player, blink at 2 Hz
  if ((!stealthMode || !hasCloakingDevice)
  ||  (controller && typeid(*controller) == typeid(HumanController) && (field->fieldClock/256 & 1))) {
    for (unsigned i=0; i<cells.size(); ++i) {
      glSetColour();
      mPush();
      glDisable(GL_TEXTURE_2D);
      cells[i]->draw(true);
      mPop();
      mPush();
      glEnable(GL_TEXTURE_2D);
      cells[i]->drawDamage();
      mPop();
    }
  }
#endif /* AB_OPENGL_14 */

  mPop();

  for (unsigned i=0; i<shields.size(); ++i)
    shields[i]->draw();
  ENDGP
}

float Ship::getRadius() const noth {
  const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_COORDS_BIT);
  return radius;
}

radar_t* Ship::getRadar() const noth {
  if (radar == &defaultRadar) return NULL;
  return radar;
}

void Ship::setRadar(radar_t* r) noth {
  radar->delClient(this);
  radar = (r? r : &defaultRadar);
}

void Ship::radarBounds(radar_t::iterator& begin, radar_t::iterator& end) noth {
  begin = radar->begin();
  end = radar->end();
}

void Ship::physicsRequire(physics_bits bits) noth {
  //Return early if possible
  if (bits == (bits & validPhysics)) return;

  //Force bits unused when fragmented to set
  if (isFragment) {
    validPhysics |= 0
                 | PHYS_CELL_THRUST_BIT
                 | PHYS_CELL_ROT_THRUST_BIT
                 | PHYS_CELL_TORQUE_BIT
                 | PHYS_CELL_CAPAC_BIT
                 | PHYS_SHIP_THRUST_BIT
                 | PHYS_SHIP_ROT_THRUST_BIT
                 | PHYS_SHIP_TORQUE_BIT
                 | PHYS_SHIP_ENGINE_INVENTORY_BIT
                 | PHYS_SHIP_WEAPON_INVENTORY_BIT
                 | PHYS_SHIP_DS_INVENTORY_BIT
                 | PHYS_SHIP_PBL_INVENTORY_BIT
                 | PHYS_SHIP_CAPAC_BIT
                 ;
    totalCapacitance = 0;
  }

  /* There are two cell-on-ship dependencies that we must handle carefully,
   * as they recurse back to this function:
   * + One or more things depend on PHYS_SHIP_COORDS_BIT; if we will be
   *   needing this, get it out of the way first (only after requiring
   *   PHYS_CELL_MASS_BIT).
   * + PHYS_CELL_DS_NEAREST_BIT depends on PHYS_SHIP_DS_INVENTORY_BIT;
   *   if we need this, perform this early.
   * Other than these two, we run all cell-specific requirements before
   * anything else.
   */

  if (!(validPhysics & PHYS_CELL_MASS_BIT) && (bits & PHYS_CELL_MASS_BITS)) {
    for (unsigned i=0; i<cells.size(); ++i)
      cells[i]->physicsRequire(PHYS_CELL_MASS_BIT);
    validPhysics |= PHYS_CELL_MASS_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_MASS_BIT) && (bits & PHYS_SHIP_MASS_BITS)) {
    mass = 0;
    for (unsigned i=0; i<cells.size(); ++i)
      mass += cells[i]->physics.mass;
    validPhysics |= PHYS_SHIP_MASS_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_COORDS_BIT) && (bits & PHYS_SHIP_COORDS_BITS)) {
    float xm = 0, ym = 0;
    for (unsigned i=0; i<cells.size(); ++i) {
      xm += cells[i]->x * cells[i]->physics.mass;
      ym += cells[i]->y * cells[i]->physics.mass;
    }
    xm /= mass;
    ym /= mass;

    /* While we recentre the coordinates, also calculate radius
     * and bounding box.
     */
    radius = 0;
    boundingSquareHalfLength = 0;
    for (unsigned i=0; i < cells.size(); ++i) {
      cells[i]->x -= xm;
      cells[i]->y -= ym;
      //Save time by delaying the sqrt(float) call.
      float r = cells[i]->x*cells[i]->x + cells[i]->y*cells[i]->y;
      if (r > radius) radius = r;
      float box = max(fabs(cells[i]->x), fabs(cells[i]->y));
      if (box > boundingSquareHalfLength)
        boundingSquareHalfLength = box;
    }
    boundingSquareHalfLength += STD_CELL_SZ; //Coordinates are the centre of cells, so compensate

    /* Move the current location the opposite direction we just adjusted
     * the cells, so that the ship does not change location.
     */
    //x -= xm*cosTheta + ym*sinTheta;
    //y -= ym*cosTheta - xm*sinTheta;

    //We collected the square radius, so sqrt it now
    radius = sqrt(radius);
    //Radius was based on the centre of the cells
    radius += STD_CELL_SZ;

    validPhysics |= PHYS_SHIP_COORDS_BIT;
  }

  if (!(validPhysics & PHYS_CELL_DS_EXIST_BIT) && (bits & PHYS_CELL_DS_EXIST_BITS)) {
    for (unsigned i=0; i<cells.size(); ++i) cells[i]->physicsRequire(PHYS_CELL_DS_EXIST_BIT);
    validPhysics |= PHYS_CELL_DS_EXIST_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_DS_INVENTORY_BIT) && (bits & PHYS_SHIP_DS_INVENTORY_BITS)) {
    cellsWithDispersionShields.clear();
    for (unsigned i=0; i<cells.size(); ++i)
      if (cells[i]->physics.hasDispersionShield)
        cellsWithDispersionShields.push_back(cells[i]);
    validPhysics |= PHYS_SHIP_DS_INVENTORY_BIT;
  }

  //We can now safely require any other CELL bits in one pass
  //We /could/ just pass the entire bits constant to physicsRequire,
  //but we want to avoid passes through the vector when possible,
  //so handle the deps here as well to see if we even need to do that
  //We also need this anyway to update our physicsValid, and doing
  //this allows Cells to take more shortcuts in Cell::physicsRequire
  physics_bits cellBits = (bits & PHYS_CELL_LOCATION_PROPERTIES_BITS?   PHYS_CELL_LOCATION_PROPERTIES_BIT : 0)
                        | (bits & PHYS_CELL_THRUST_BITS?                PHYS_CELL_THRUST_BIT : 0)
                        | (bits & PHYS_CELL_TORQUE_BITS?                PHYS_CELL_TORQUE_BIT : 0)
                        | (bits & PHYS_CELL_ROT_THRUST_BITS?            PHYS_CELL_ROT_THRUST_BIT : 0)
                        | (bits & PHYS_CELL_COOLING_BITS?               PHYS_CELL_COOLING_BIT : 0)
                        | (bits & PHYS_CELL_HEAT_BITS?                  PHYS_CELL_HEAT_BIT : 0)
                        | (bits & PHYS_CELL_POWER_BITS?                 PHYS_CELL_POWER_BIT : 0)
                        | (bits & PHYS_CELL_POWER_PROD_BITS?            PHYS_CELL_POWER_PROD_BIT : 0)
                        | (bits & PHYS_CELL_CAPAC_BITS?                 PHYS_CELL_CAPAC_BIT : 0)
                        | (bits & PHYS_CELL_DS_NEAREST_BITS?            PHYS_CELL_DS_NEAREST_BIT : 0)
                        | (bits & PHYS_CELL_REINFORCEMENT_BITS?         PHYS_CELL_REINFORCEMENT_BIT : 0)
                        ;
  //Remove anything we already have
  cellBits &= ~validPhysics;

  if (cellBits)
    for (unsigned i=0; i<cells.size(); ++i)
      cells[i]->physicsRequire(cellBits);
  validPhysics |= cellBits;

  //Now take care of everything else

  if (!(validPhysics & PHYS_SHIP_INERTIA_BIT) && (bits & PHYS_SHIP_INERTIA_BITS)) {
    angularInertia = 0;
    for (unsigned i=0; i<cells.size(); ++i) {
      angularInertia += INERTIA_FUDGE_FACTOR * cells[i]->physics.mass
                      * cells[i]->physics.distance * cells[i]->physics.distance;
      assert(angularInertia == angularInertia);
    }

    //To prevent divides by 0 and similar, enforce a minimum inertia
    float minin = cells[0]->physics.mass * STD_CELL_SZ/2;
    if (angularInertia < minin) angularInertia = minin;
    assert(angularInertia == angularInertia);

    validPhysics |= PHYS_SHIP_INERTIA_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_ENGINE_INVENTORY_BIT) && (bits & PHYS_SHIP_ENGINE_INVENTORY_BITS)) {
    engineInfo.list.clear();
    for (unsigned i=0; i<cells.size(); ++i)
      if (cells[i]->systems[0] && cells[i]->systems[0]->clazz == Classification_Engine)
        engineInfo.list.push_back((Engine*)cells[i]->systems[0]);
      else if (cells[i]->systems[1] && cells[i]->systems[1]->clazz == Classification_Engine)
        engineInfo.list.push_back((Engine*)cells[i]->systems[1]);

    validPhysics |= PHYS_SHIP_ENGINE_INVENTORY_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_THRUST_BIT) && (bits & PHYS_SHIP_THRUST_BITS)) {
    thrustX = 0;
    thrustY = 0;

    for (unsigned i=0; i<engineInfo.list.size(); ++i) {
      thrustX += engineInfo.list[i]->container->physics.thrustX;
      thrustY += engineInfo.list[i]->container->physics.thrustY;
    }

    thrustMag = sqrt(thrustX*thrustX + thrustY*thrustY);

    validPhysics |= PHYS_SHIP_THRUST_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_TORQUE_BIT) && (bits & PHYS_SHIP_TORQUE_BITS)) {
    thrustTorque = 0;
    list<Cell*> e;
    bool requiresPairScanning = true;
    for (unsigned i=0; i<engineInfo.list.size(); ++i) {
      if (engineInfo.list[i]->container->physics.torquePair)
        requiresPairScanning = false;
      else
        e.push_back(engineInfo.list[i]->container);
    }

    /* For each Cell, find the best-negative matching Cell that
     * occurs later in the list, for matching torque. If they are
     * +/- 10% of each other, set the matching Cell for the torquePair
     * to both and remove both from the list.
     * This only has to be done once for the life of the Ship.
     */
    if (requiresPairScanning) {
      list<Cell*>::iterator it = e.begin();
      while (it != e.end()) {
        list<Cell*>::iterator match = e.end();
        float matchQuality = FLT_MAX;
        list<Cell*>::iterator curr = it;
        while (++curr != e.end()) {
          if (((*curr)->physics.torque < 0 && (*it)->physics.torque < 0)
          ||  ((*curr)->physics.torque > 0 && (*it)->physics.torque > 0))
            continue;

          float quality = fabs(fabs((*curr)->physics.torque) - fabs((*it)->physics.torque));
          if (quality < matchQuality) {
            matchQuality = quality;
            match = curr;
          }
        }

        if (match != e.end()
        &&  matchQuality / max(fabs((*it)->physics.torque),
                              max(fabs((*match)->physics.torque),1.0e-12f)) < 0.1f) {
          //This is a match
          //Remove both
          (*it)->physics.torquePair = *match;
          (*match)->physics.torquePair = *it;
          e.erase(match);
          it = e.erase(it);
        } else {
          //No match, move to next
          ++it;
        }
      }
    }

    for (list<Cell*>::iterator it = e.begin(); it != e.end(); ++it)
      thrustTorque += (*it)->physics.torque;

    validPhysics |= PHYS_SHIP_TORQUE_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_ROT_THRUST_BIT) && (bits & PHYS_SHIP_ROT_THRUST_BITS)) {
    rotationalThrust = 0;
    for (unsigned i = 0; i < engineInfo.list.size(); ++i)
      rotationalThrust += engineInfo.list[i]->container->physics.rotationalThrust;

    validPhysics |= PHYS_SHIP_ROT_THRUST_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_COOLING_BIT) && (bits & PHYS_SHIP_COOLING_BITS)) {
    heatingCount = heating_count(this);
    if (heatingCount) {
      //rawCoolingMult = cooling_amount(this);
      rawCoolingMult = 1;
      for (unsigned i=0; i<cells.size(); ++i)
        rawCoolingMult += cells[i]->physics.cooling;
      coolingMult = rawCoolingMult / sqrt((float)heatingCount);
    } else {
      rawCoolingMult = 1;
      coolingMult = 1;
    }

    validPhysics |= PHYS_SHIP_COOLING_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_POWER_BIT) && (bits & PHYS_SHIP_POWER_BITS)) {
    powerUC = powerUT = powerSC = powerST = 0;
    ppowerU = ppowerS = 0;
    for (unsigned i=0; i < cells.size(); ++i) {
      const Cell::PhysicalInformation& p(cells[i]->physics);
      powerUC += p.powerUC;
      powerUT += p.powerUT;
      powerSC += p.powerSC;
      powerST += p.powerST;
      ppowerU += p.ppowerU;
      ppowerS += p.ppowerS;
    }

    updateCurrPower();

    validPhysics |= PHYS_SHIP_POWER_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_CAPAC_BIT) && (bits & PHYS_SHIP_CAPAC_BITS)) {
    totalCapacitance = 0;
    if (!isStealth() && !isFragment) {
      for (unsigned i=0; i < cells.size(); ++i)
        totalCapacitance += cells[i]->physics.capacitance;
    }

    validPhysics |= PHYS_SHIP_CAPAC_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_WEAPON_INVENTORY_BIT) && (bits & PHYS_SHIP_WEAPON_INVENTORY_BITS)) {
    for (unsigned i=0; i<lenof(weaponsMap); ++i) {
      weaponsMap[i].clear();
      weapon_enumerate(this,(Weapon)i,weaponsMap[i]);
    }

    validPhysics |= PHYS_SHIP_WEAPON_INVENTORY_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_SHIELD_INVENTORY_BIT) && (bits & PHYS_SHIP_SHIELD_INVENTORY_BITS)) {
    shields.clear();
    shield_enumerate(this, shields);
    validPhysics |= PHYS_SHIP_SHIELD_INVENTORY_BIT;
  }

  if (!(validPhysics & PHYS_SHIP_PBL_INVENTORY_BIT) && (bits & PHYS_SHIP_PBL_INVENTORY_BITS)) {
    plasmaBurstLauncherInfo.count = 0;
    for (unsigned i=0; i<cells.size(); ++i) for (unsigned s=0; s<2; ++s) {
      if (cells[i]->systems[s]) {
        PlasmaBurstLauncher* p = NULL;
        p = dynamic_cast<PlasmaBurstLauncher*>(cells[i]->systems[s]);
        if (p)
          ++plasmaBurstLauncherInfo.count;
      }
    }

    validPhysics |= PHYS_SHIP_PBL_INVENTORY_BIT;
  }

  assert((validPhysics & bits) == bits);
}

Cell* Ship::nearestDispersionShield(const Cell* cell, float& dist) const noth {
  if (isFragment) return NULL;
  assert(validPhysics & PHYS_SHIP_DS_INVENTORY_BIT);

  Cell* nearest = NULL;
  for (unsigned i=0; i<cellsWithDispersionShields.size(); ++i) {
    float dx = cell->x - cellsWithDispersionShields[i]->x;
    float dy = cell->y - cellsWithDispersionShields[i]->y;
    float dsq = dx*dx + dy*dy;
    //Work with square distance until the end to save the sqrt calls
    if (!nearest || dsq < dist) {
      nearest = cellsWithDispersionShields[i];
      dist = dsq;
    }
  }

  if (nearest)
    dist = sqrt(dist);

  return nearest;
}

void Ship::refreshUpdates() noth {
  cellUpdateFunctions.clear();
  #define SYS(n) if (cells[i]->systems[n] && \
                     cells[i]->systems[n]->update) { \
                   SystemUpdate su={cells[i]->systems[n], \
                                    cells[i]->systems[n]->update}; \
                   cellUpdateFunctions.push_back(su); \
                 }
  for (unsigned i=0; i<cells.size(); ++i) {
    SYS(0)
    SYS(1)
  }
  #undef SYS
}

void Ship::accel(float time) noth {
  float thrustMul=thrustPercent*time/mass;
  //Axis-alligned velocity delta
  float dvx = thrustMul*thrustX;
  float dvy = thrustMul*thrustY;
  //Adjust according to current cosine/sine
  vx += dvx*cosTheta - dvy*sinTheta;
  vy += dvy*cosTheta + dvx*sinTheta;

  //Handle off-balance engines
  vtheta-=thrustPercent*thrustTorque/angularInertia*time;
}

void Ship::decel(float time) noth {
  if (0 == thrustMag) return;

  //The ship can half its speed in the time it would take to accelerate
  //the given amount
  float speed=sqrt(vx*vx + vy*vy);
  float accelTime=speed*mass/thrustMag;
  float factor=1;
  if (accelTime >= 1)
    factor=(accelTime-time)/accelTime;

  if (accelTime<1 || factor>1 || factor<=0) {
    vx=vy=0;
  } else {
    vx*=factor;
    vy*=factor;
  }
}

void Ship::spin(float amount) noth {
  vtheta+=amount * SPIN_UP *
          rotationalThrust /
          angularInertia;
}

void Ship::setStealthMode(bool mode) noth {
  if (mode==stealthMode) return;

  stealthMode=mode;
  //Tell all systems about the switch IF any current transition
  //was completed
  if (stealthCounter==STEALTH_COUNTER_MIN
  ||  stealthCounter==STEALTH_COUNTER_MAX) {
    for (unsigned i=0; i<cells.size(); ++i) {
      if (cells[i]->systems[0])
        cells[i]->systems[0]->stealthTransition();
      if (cells[i]->systems[1])
        cells[i]->systems[1]->stealthTransition();
    }
  }

  //See if we have a cloaking device
  if (mode) {
    unsigned devs = 0;
    for (unsigned i=0; i<cells.size(); ++i) for (unsigned s=0; s<2; ++s)
      if (cells[i]->systems[s] && cells[i]->systems[s]->clazz == Classification_Cloak)
        ++devs;

    hasCloakingDevice = (devs >= cells.size()/5 && devs >= 1);
  }

  updateCurrPower();
  for (unsigned i=0; i < cells.size(); ++i)
    cells[i]->physicsClear(PHYS_CELL_COOLING_BITS | PHYS_CELL_THRUST_BITS
                          |PHYS_CELL_ROT_THRUST_BITS | PHYS_CELL_THRUST_BITS);
  physicsClear(PHYS_SHIP_ENGINE_INVENTORY_BITS | PHYS_SHIP_SHIELD_INVENTORY_BITS | PHYS_SHIP_CAPAC_BITS);
}

float Ship::getRotationRate() const noth {
  const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_ROT_THRUST_BIT | PHYS_SHIP_INERTIA_BIT);
  float vt=0;
  //Simulate in 1000 10-ms steps
  for (unsigned i=0; i<1000; ++i) {
    vt+=STD_ROT_RATE*10.0f * SPIN_UP *
        rotationalThrust /
        angularInertia;
    vt*=(angularInertia/rotationalThrust-10.0f*SPIN_DOWN) /
        (angularInertia/rotationalThrust);
  }
  return vt;
}

float Ship::getRotationAccel() const noth {
  const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_ROT_THRUST_BIT | PHYS_SHIP_INERTIA_BIT);
  return STD_ROT_RATE*SPIN_UP*rotationalThrust/angularInertia;
}

float Ship::getUncontrolledRotationAccel() const noth {
  const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_ROT_THRUST_BIT | PHYS_SHIP_INERTIA_BIT
                                         |PHYS_SHIP_TORQUE_BIT);
  return -thrustPercent*thrustTorque/angularInertia;
}

const vector<CollisionRectangle*>* Ship::getCollisionBounds() noth {
  physicsRequire(PHYS_SHIP_COORDS_BIT
                |PHYS_CELL_LOCATION_PROPERTIES_BIT
                |PHYS_SHIP_SHIELD_INVENTORY_BIT);
  if (!collisionTree.hasData()) {
    float* tzd = temporaryZero();
    vector<CollisionRectangle*> vec;
    for (unsigned int i=0; i<cells.size(); ++i) {
      CollisionRectangle* cr=cells[i]->getCollisionBoundsBeta();
      if (cr) vec.push_back(cr);
    }
    restoreFromZero(tzd);

    collisionTree.setData(&vec[0], vec.size());
    collisionTree.reset(x, y, cosTheta, sinTheta);
  }
  collisionBounds = collisionTree.get();
  for (unsigned i=0; i<shields.size(); ++i)
    shields[i]->addCollisionBounds(collisionBounds);
  return &collisionBounds;
}

pair<float,float> Ship::cellCoord(const Ship* parent, const Cell* cell, float x, float y) noth {
  float ptcos = parent->cosTheta;
  float ptsin = parent->sinTheta;
  float cellTheta=cell->getT()*pi/180 + parent->theta;
  return make_pair(parent->x + x*cos(cellTheta) - y*sin(cellTheta) +
                               cell->x*ptcos -
                               cell->y*ptsin,
                   parent->y + y*cos(cellTheta) + x*sin(cellTheta) +
                               cell->y*ptcos +
                               cell->x*ptsin);
}
pair<float,float> Ship::cellCoord(const Ship* parent, const Cell* cell) noth {
  float ptcos = parent->cosTheta;
  float ptsin = parent->sinTheta;
  return make_pair(parent->x + cell->x*ptcos -
                               cell->y*ptsin,
                   parent->y + cell->y*ptcos +
                               cell->x*ptsin);
}

pair<float,float> Ship::getCellVelocity(const Cell* cell) const noth {
  float dist=cell->physics.distance,
       angle=cell->physics.angle + theta;
  //Since vtheta is in radians, it is 2*pi*<percentage>
  return make_pair(this->vx + cos(angle+pi/2)*dist*vtheta,
                   this->vy + sin(angle+pi/2)*dist*vtheta);
}

bool Ship::drawPower(float amount) noth {
  if (powerTest) {
    powerTestSum+=amount*1000;
    return false;
  }

  currentCapacitance-=amount*1000;
  if (currentCapacitance<0) {
    currentCapacitance=0;
    return false;
  }
  return true;
}

void Ship::startTest() noth {
  powerTest=true;
  powerTestSum=0;
}
float Ship::endTest() noth {
  powerTest=false;
  return powerTestSum/1000;
}

CollisionResult Ship::checkCollision(GameObject* other) noth {
  return MaybeCollision;
}

bool Ship::collideWith(GameObject* other) noth {
  physicsRequire(PHYS_SHIP_SHIELD_INVENTORY_BIT
                |PHYS_SHIP_MASS_BIT
                |PHYS_SHIP_INERTIA_BIT
                |PHYS_SHIP_COORDS_BIT
                |PHYS_SHIP_DS_INVENTORY_BIT
                );
  if (other == this) {
    //Special handling
    //First, inform all shields
    for (unsigned i=0; i<shields.size(); ++i)
      shields[i]->collideWith(shields[i]);
    //Notify those concerned about our death,
    //and then die gracefully
    death(true);
    return false;
  }

  if (typeid(*other)==typeid(Blast)) {
    Blast* blast=(Blast*)other;
    //Compensate for Blasts from the other decoration
    if (blast->isDecorative() != this->isDecorative()) return true;
    /* If we are remote, just calculate
     * velocity change and forward the blast
     * to the remote peer.
     */
    if (isRemote && shipDamageGeraet && blast->causesDamage()
    &&  blast->isDecorative() == this->isDecorative())
      shipDamageGeraet->shipBlastCollision(this, blast);

    /* Since we integrate shield collision with our own,
     * we must pass this on to all Shields as well (it
     * is OK if no collision actually occurred).
     *
     * (Collide the shields even if remote, so that we don't
     * have to wait for confirmation.)
     */
    for (unsigned i=0; i<shields.size(); ++i) {
      /* A Shield will never die from collision,
       * so it's OK to ignore the return value.
       */
      shields[i]->collideWith(other);
      /* Nothing that collides with shields needs to know
       * /that/ specifically. All collideables work fine
       * just believing that they hit a Ship.
       */
      //other->collideWith(shields[i]);
    }

    bool rootDestroyed=false;
    float dx, dy;
    pair<float,float> edge;
    Cell* maxStrengthCell=NULL;
    float maxStrength=0, totalStrength=0;

    for (unsigned int i=0; i<cells.size(); ++i) {
      if (cells[i]->isEmpty) continue;
      pair<float,float> cc=cellCoord(this, cells[i]);

      edge=closestEdgePoint(*cells[i]->getCollisionBounds(),
                            make_pair(blast->getX(), blast->getY()));
      dx=edge.first-blast->getX(), dy=edge.second-blast->getY();
      //See if it would be better to use the centre of the cell than the edge
      float cdx = cc.first-blast->getX(), cdy=cc.second-blast->getY();
      if (cdx*cdx + cdy*cdy < dx*dx + dy*dy) {
        edge=cc;
        dx=cdx;
        dy=cdy;
      }

      /* This code is now handled generically in applyCollision(), but is kept
       * here to further explain how it works in this context.
       */
      /**
       * //We actually care about the edge->ship, not cell->ship
       * cellToShipAngle=atan2(edge.second-centre.second,
       *                       edge.first -centre.first);
       * //We should consider the blast to be comming from the angle at
       * //which is the closest point
       * blastToCellAngle=atan2(blast->getY()-edge.second,
       *                        blast->getX()-edge.first);
       **/

      float distance=sqrt(dx*dx + dy*dy);

      float strength=blast->getStrength(distance),
            movementStrength=blast->getStrength(distance/2);
      if (movementStrength==0) continue;
      totalStrength += strength;
      if (strength>maxStrength) {
        maxStrength=strength;
        maxStrengthCell = cells[i];
      }
      //Velocity adjustments
      applyCollision(edge.first, edge.second, strength, blast->getX(),
                     blast->getY());
    }

    effects->impact(totalStrength/mass);
    if (soundEffects && maxStrengthCell)
      shipDynamicEvent(new audio::Virtual<audio::shipImpact>,
                       SHIP_IMPACT_LEN, maxStrengthCell,
                       totalStrength * IMPACT_VOLUME_MUL);

    //Handle damage
    float totalDamage = 0;
    bool destruction=false;
    float damageXOff=0, damageYOff=0;
    float maxCellDamage = 0;
    for (unsigned int i=0; i<cells.size() && blast->causesDamage(); ++i) {
      if (cells[i]->isEmpty) continue;
      pair<float,float> cc=closestEdgePoint(*cells[i]->getCollisionBounds(),
                                            make_pair(blast->getX(),
                                                      blast->getY()));
      float dx=cc.first-blast->getX(), dy=cc.second-blast->getY();
      float dist=sqrt(dx*dx + dy*dy);
      float strength=blast->getStrength(dist);
      if (!isRemote && strength > 0) {
        //Check for possible damage reduction
        cells[i]->physicsRequire(PHYS_CELL_DS_NEAREST_BIT);
        if (cells[i]->physics.distanceDS >= 0
        &&  !stealthMode
        &&  !shieldsDeactivated
        &&  !isFragment) {
          float maxRedByDist = DAM_DISP_MAX /
              (1 + DAM_DISP_DIST_MUL*cells[i]->physics.distanceDS/STD_CELL_SZ);
          float maxRedByCap = currentCapacitance / DAM_DISP_CAP_MUL;
          float maxRedByTemp = (MAX_TEMP - heatInfo.temperature) /
                               DAM_DISP_TEMP_MUL;
          float maxRedByPDist = strength /
              (1 + DAM_DISP_DIST_MUL*cells[i]->physics.distanceDS/STD_CELL_SZ);
          float reduction = min(min(maxRedByDist, maxRedByCap),
                                min(maxRedByTemp, maxRedByPDist));

          //Apply if possible
          if (reduction > 0) {
            strength -= reduction;
            currentCapacitance -= reduction * DAM_DISP_CAP_MUL;
            heatInfo.temperature += reduction * DAM_DISP_TEMP_MUL;
          }
          reduction += 1;
        }

        if (strength > maxCellDamage) {
          maxCellDamage = strength;
          damageXOff = cells[i]->getX();
          damageYOff = cells[i]->getY();
        }
        totalDamage += strength;
        if (!cells[i]->applyDamage(strength*damageMultiplier, blast->blame)) {
          preremove(cells[i]);
          CellFragment::spawn(cells[i], blast);

          //Replace with several empty cells
          destruction=true;
          for (unsigned n=0; n<cells[i]->numNeighbours(); ++n) {
            if (cells[i]->neighbours[n]) {
              int index=cells[i]->neighbours[n]->getNeighbour(cells[i]);
              EmptyCell* ec=new EmptyCell(this, cells[i]->neighbours[n]);
              if (highQuality && !isFragment) field->add(new PlasmaFire(ec));
              cells[i]->neighbours[n]->neighbours[index]=ec;
              cells.push_back(ec);
            }
          }
          //If this was the bridge, we're dead
          if (i==0) {
            currentCapacitance=0;
            isFragment=decorative=true;
            rootDestroyed=true;
          }

          //Remove networking entry
          if (networkCells.size() > 0)
            networkCells[cells[i]->netIndex] = NULL;

          //Remove cell
          delete cells[i];
          if (renderer) renderer->cellRemoved(cells[i]);
          cells.erase(cells.begin() + i--);
        }
      }
    }

    //Assign blame for this damage
    float minBlamedDamage = damageBlame[0].damage;
    unsigned blameix = 0;
    for (unsigned i=0; i<lenof(damageBlame); ++i) {
      if (damageBlame[i].blame == blast->blame) {
        blameix = i;
        break;
      }
      if (damageBlame[i].damage < minBlamedDamage) {
        minBlamedDamage = damageBlame[i].damage;
        blameix = i;
      }
    }
    if (damageBlame[blameix].blame != blast->blame) {
      damageBlame[blameix].blame = blast->blame;
      damageBlame[blameix].damage = totalDamage;
    } else {
      damageBlame[blameix].damage += totalDamage;
    }
    if (controller)
      controller->damage(totalDamage, damageXOff, damageYOff);

    if (destruction) {
      //Handle possible fragments
      vector<Cell*> bridgeAttached;
      //If the root was destroyed, create an entirely
      //new ship, by making bridgeAttached be empty
      if (!rootDestroyed)
        cells[0]->getAdjoined(bridgeAttached);
      if (bridgeAttached.size()!=cells.size()) {
        //We've split...
        //Use a non-damaging blast to blow fragments apart
        Blast blowUp(blast, false);
        //Isolate each fragment and create new ships
        for (unsigned int i=0; i<cells.size(); ++i) {
          bool attached=false;
          for (unsigned int j=0; j<bridgeAttached.size() && !attached; ++j)
            if (bridgeAttached[j]==cells[i])
              attached=true;
          if (attached) continue;

          //Fragment
          //Find all cells attached and remove from ours,
          //then create new ship
          Ship* frag=new Ship(field);
          frag->isFragment=frag->decorative=true;
          /* We don't call okToDecorate because then
           * the fragment will be doubly-exposed to
           * the blast (ie, it will collide with the
           * secondary decorative blast that was
           * automatically created). okToDecorate()
           * is not called by GameField until AFTER
           * the next decorative update, allowing
           * the second blast to expire first.
           */
          cells[i]->getAdjoined(frag->cells);
          for (unsigned int j=0; j<frag->cells.size(); ++j) {
            //Subtract from physics
            preremove(frag->cells[j]);
            //Transfer ownership
            frag->cells[j]->parent=frag;
            if (frag->cells[j]->systems[0]) {
              frag->cells[j]->systems[0]->parent=frag;
              frag->cells[j]->systems[0]->parentChanged();
              frag->cells[j]->systems[0]->detectPhysics();
            }
            if (frag->cells[j]->systems[1]) {
              frag->cells[j]->systems[1]->parent=frag;
              frag->cells[j]->systems[1]->parentChanged();
              frag->cells[j]->systems[1]->detectPhysics();
            }
            for (unsigned int k=0; k<cells.size(); ++k) {
              if (cells[k]==frag->cells[j]) {
                if (renderer) renderer->cellRemoved(cells[k]);
                if (networkCells.size() && !cells[k]->isEmpty)
                  networkCells[cells[k]->netIndex] = NULL;
                cells.erase(cells.begin() + k);

                //Looks odd, but i is unsigned...
                if (k<=i && i<(((unsigned int)0)-1)) --i;
                --k;
                goto cellRemoved;
              }
            }

            cerr << "Unable to find a match for cell: " << frag->cells[j]
                 << endl;
            exit(EXIT_PROGRAM_BUG);

            cellRemoved:;
          }
          //Kill the ship if it is only EmptyCells
          bool isEmpty=true;
          for (unsigned int j=0; j<frag->cells.size() && isEmpty; ++j) {
            if (!frag->cells[j]->isEmpty) isEmpty=false;
          }
          if (isEmpty) {
            delete frag;
            continue;
          }

          //Detect the fragment's physics so we can position it so it looks
          //like it broke off
          float rootTheta=frag->cells[0]->getT()*pi/180; //Since the root may be rotated relative to the mothership
          //Get the old location of the root so we can adjust later
          pair<float,float> rootPos=cellCoord(this, frag->cells[0]);
          for (unsigned int j=0; j<frag->cells.size(); ++j) {
            frag->cells[j]->disorient();
            frag->cells[j]->physicsClear(PHYS_CELL_LOCATION_PROPERTIES_BITS);
          }
          frag->cells[0]->orient();
          frag->theta=rootTheta+this->theta;
          frag->cosTheta=cos(frag->theta);
          frag->sinTheta=sin(frag->theta);
          //Temporary location
          frag->x=frag->y=0;
          frag->reinforcement=this->reinforcement;
          //Move so it appears in the correct location
          frag->physicsRequire(PHYS_SHIP_COORDS_BIT | PHYS_CELL_LOCATION_PROPERTIES_BIT);
          pair<float,float> offset=cellCoord(frag, frag->cells[0]);
          frag->x = rootPos.first - offset.first;
          frag->y = rootPos.second - offset.second;
          frag->vtheta=this->vtheta;
          frag->colourR=this->colourR;
          frag->colourG=this->colourG;
          frag->colourB=this->colourB;
          frag->insignia=this->insignia;
          float dx=frag->x-this->x, dy=frag->y-this->y;
          float centreDist=sqrt(dx*dx + dy*dy);
          float centreAngle=(centreDist!=0? atan2(dy, dx) : 0);
          frag->vx = this->vx + centreDist*vtheta*cos(centreAngle+theta);
          frag->vy = this->vy + centreDist*vtheta*sin(centreAngle+theta);
          //Blow apart
          frag->collideWith(&blowUp);
          /* There is a possibility of the fragment being out of bounds, so have it not be added
           * until after collision detection. */
          field->addBegin(frag);
        }
      }
      if (cells.size()!=bridgeAttached.size()) {
        cerr << "FATAL: Not all detached cells accounted for.\n"
        "We have " << cells.size() << ", but should have " << bridgeAttached.size() << endl;
        cerr << "Us=" << this << endl;
        for (unsigned int i=0; i<cells.size(); ++i) cout << i << " " << cells[i] << ' ' <<
                                                            cells[i]->getX() << ' ' <<
                                                            cells[i]->parent << endl;
        for (unsigned int i=0; i<bridgeAttached.size(); ++i) cout << i << " " << bridgeAttached[i] << endl;
        exit(EXIT_PROGRAM_BUG);
      }
      //See if we should exist
      //We shouldn't if we're only empty cells
      bool isEmpty=true;
      for (unsigned int i=0; i<cells.size() && isEmpty; ++i)
        if (!cells[i]->isEmpty) isEmpty=false;
      if (isEmpty) {
        //Just return false and we'll be kindly deleted
        death(true);
        return false;
      } else {
        if (rootDestroyed) death(false);
        float oldx=x, oldy=y;
        //We will still float around even if we are a fragment, so redetect
        //physics and return true
        //The root will never be rotated, so we don't need to check that.
        pair<float,float> oldCoord=cellCoord(this, cells[0]);
        for (unsigned i=0; i<cells.size(); ++i) {
          cells[i]->disorient();
          cells[i]->physicsClear(PHYS_CELL_LOCATION_PROPERTIES_BITS);
        }
        cells[0]->orient();
        physicsRequire(PHYS_SHIP_COORDS_BIT | PHYS_CELL_LOCATION_PROPERTIES_BIT);
        pair<float,float> newCoord = cellCoord(this, cells[0]);
        //Compensate for reorientation
        x += oldCoord.first-newCoord.first;
        y += oldCoord.second-newCoord.second;

        float cdx=oldx-x, cdy=oldy-y;
        float cdt=atan2(cdy, cdx);
        float cdd=sqrt(cdx*cdx + cdy*cdy);
        //Adjust velocity according to rotation speed
        vx-=vtheta*cdd*cos(cdt+pi/2);
        vy-=vtheta*cdd*sin(cdt+pi/2);

        refreshUpdates();
        if (soundEffects)
          audio::ShipMixer::setShip(&cells[0], cells.size(), reinforcement);
      }
    }

    //If we are remote, send notification
    //if (isRemote)
      //field->remoteCollision(this, blast);
  } else if (typeid(*other) == typeid(ParticleBurst) && !isRemote) {
    const CollisionRectangle& br(*other->getCollisionBounds()->operator[](0));
    for (unsigned i=0; i<cells.size(); ++i) {
      const CollisionRectangle* cr=cells[i]->getCollisionBounds();
      if (!cr) continue;
      if (rectanglesCollide(*cr, br)) {
        //Apply effects
        Cell* cell=cells[i];
        bool change=false;
        if (cell->systems[0] && !cell->systems[0]->particleBeamCollision((ParticleBurst*)other)) {
          delete cell->systems[0];
          cell->systems[0]=NULL;
          change=true;
        }
        if (cell->systems[1] && !cell->systems[1]->particleBeamCollision((ParticleBurst*)other)) {
          delete cell->systems[1];
          cell->systems[1]=NULL;
          change=true;
        }
        if (change) {
          cell->physicsClear(PHYS_CELL_MASS_BITS
                            |PHYS_CELL_POWER_BITS
                            |PHYS_CELL_POWER_PROD_BITS);
          refreshUpdates();
        }
        return true; //Only apply to one cell
      }
    }
  }

  return true;
}

void Ship::applyCollision(float px, float py, float strength, float ox, float oy) noth {
  //Velocity adjustments
  static const float move_factor=0.01f;
  float sdx=x - px,
        sdy=y - py;
  float sdist=sqrt(sdx*sdx + sdy*sdy);

  float pangle = atan2(py-y,  px-x);
  float oangle = atan2(oy-py, ox-px);

  vx-=strength/mass * cos(oangle) * move_factor;
  vy-=strength/mass * sin(oangle) * move_factor;
  //The angular acceleration is relative to the angle
  //A parallel (directly into the ship) force adds no rotation,
  //but a perpindicular adds the most. Also, further distance increases force
  static const float rot_factor=move_factor;
  vtheta-=strength*sdist*sin(oangle-pangle)/angularInertia * 2*pi * rot_factor;
  if (vtheta != vtheta) {
    cout << "Vtheta is NaN!" << endl;
  }
}

const NebulaResistanceElement* Ship::getNebulaResistanceElements() noth {
  //Count the number of cells to include; omit empty cells
  unsigned n=0;
  for (unsigned i=0; i<cells.size(); ++i)
    if (!cells[i]->isEmpty)
      ++n;
  nebulaResistanceElements.resize(n);
  unsigned ix=0;
  for (unsigned i=0; i<cells.size(); ++i) {
    if (!cells[i]->isEmpty) {
      pair<float,float> pos = cellCoord(this,cells[i]), vel = getCellVelocity(cells[i]);
      nebulaResistanceElements[ix].px = pos.first;
      nebulaResistanceElements[ix].py = pos.second;
      nebulaResistanceElements[ix].cx = x - pos.first;
      nebulaResistanceElements[ix].cy = y - pos.second;
      nebulaResistanceElements[ix].vx = vel.first;
      nebulaResistanceElements[ix].vy = vel.second;
      ++ix;
    }
  }
  return &nebulaResistanceElements[0];
}

unsigned Ship::getNumNebulaResistanceElements() const noth {
  return nebulaResistanceElements.size();
}

void Ship::spontaneouslyDie() noth {
  if (isFragment || isRemote) return;
  isFragment=true;
  diedSpontaneously = true;
  death(false);
}

const vector<ShipSystem*>& Ship::getWeapons(unsigned w) const noth {
  const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_WEAPON_INVENTORY_BIT);
  if (isFragment) const_cast<Ship*>(this)->weaponsMap[w].clear();
  return weaponsMap[w];
}

void Ship::cellDamaged(Cell* c) noth {
  if (renderer)
    renderer->cellDamage(c);
}

void Ship::cellChanged(Cell* c) noth {
  if (renderer)
    renderer->cellChanged(c);
}

void Ship::destroyGraphicsInfo() noth {
  if (renderer) delete renderer;
  renderer = NULL;
}

void Ship::death(bool del) noth {
  if (shipExistenceFailure) shipExistenceFailure(this, del);
  if (controller) delete controller;
  controller=NULL;
  effects=&nullEffectsHandler;
  radar->delClient(this);
  radar->superPurge(this);
  if (soundEffects)
    disableSoundEffects();

  //Notify any Ship Controllers that may care about our death
  for (unsigned i=0; i<field->size(); ++i) {
    GameObject* go = field->at(i);
    if (go->getClassification() == ClassShip) {
      Ship* s = (Ship*)go;
      if (s->controller)
        s->controller->otherShipDied(this);
    }
  }
}

void Ship::updateCurrPower() noth {
  signed currPowerDelta;
  float thrustMul = (thrustOn? thrustPercent : 0) + (brakeOn? 0.5f*thrustPercent : 0);
  if (isStealth()) {
    currPowerDelta = powerSC + thrustMul*powerST;
    currPowerProd = ppowerS;
  } else {
    currPowerDelta = powerUC + thrustMul*powerUT;
    currPowerProd = ppowerU;
  }
  currPowerDrain = currPowerProd + currPowerDelta;
  if (hasCloakingDevice && stealthMode)
    currPowerDrain += CLOAK_POWER_REQ*cells.size();
}

void Ship::preremove(Cell* cell) noth {
  if (cell->isEmpty) return; //Nothing to do with EmptyCells

  //PHYS_SHIP_COORDS_BIT: can't be patched
  physicsClear(PHYS_CELL_LOCATION_PROPERTIES_BITS | PHYS_SHIP_COORDS_BITS);
  //PHYS_SHIP_MASS_BIT: subtract cell mass
  if (validPhysics & PHYS_SHIP_MASS_BIT)
    mass -= cell->physics.mass;
  //PHYS_SHIP_INERTIA_BIT: depends on PHYS_SHIP_COORDS_BIT, can't be patched
  //PHYS_SHIP_THRUST_BIT: subtract thrust components
  if ((validPhysics & PHYS_SHIP_THRUST_BIT) && !isFragment) {
    thrustX -= cell->physics.thrustX;
    thrustY -= cell->physics.thrustY;
    thrustMag = sqrt(thrustX*thrustX + thrustY*thrustY);
  }
  //PHYS_SHIP_TORQUE_BIT: depends on PHYS_SHIP_COORDS_BIT, can't be patched
  //But we do need to check for now-invalidated pairing
  if (cell->physics.torquePair)
    cell->physics.torquePair = cell->physics.torquePair->physics.torquePair = NULL;
  //PHYS_SHIP_ROT_THRUST_BIT: depends on PHYS_SHIP_COORDS_BIT, can't be patched
  //PHYS_SHIP_COOLING_BIT: adjust values and recalculate
  if ((validPhysics & PHYS_SHIP_COOLING_BIT) && !isFragment) {
    if (cell->systems[0]) heatingCount -= cell->systems[0]->heating_count();
    if (cell->systems[1]) heatingCount -= cell->systems[1]->heating_count();
    rawCoolingMult -= cell->physics.cooling;
    if (heatingCount)
      coolingMult = rawCoolingMult / sqrt((float)heatingCount);
    else
      coolingMult = 1;
  }
  //PHYS_SHIP_POWER_BIT: subtract individual values and recalc current
  if ((validPhysics & PHYS_SHIP_POWER_BIT) && !isFragment) {
    powerST -= cell->physics.powerST;
    powerSC -= cell->physics.powerSC;
    powerUT -= cell->physics.powerUT;
    powerUC -= cell->physics.powerUC;
    ppowerS -= cell->physics.ppowerS;
    ppowerU -= cell->physics.ppowerU;
    updateCurrPower();
  }
  //PHYS_SHIP_CAPAC_BIT: subtract value
  if ((validPhysics & PHYS_SHIP_CAPAC_BIT) && !isStealth() && !isFragment)
    totalCapacitance -= cell->physics.capacitance;
  //PHYS_CELL_DS_EXIST_BIT: if exists, unlink, remove from inventory,
  //unset PHYS_CELL_DS_NEAREST_BIT for all formerly linked cells
  if (!isFragment) {
    if ((cell->physics.valid & PHYS_CELL_DS_EXIST_BIT) &&
        cell->physics.hasDispersionShield) {
      cell->clearDSChain();
      cellsWithDispersionShields.erase(find(cellsWithDispersionShields.begin(),
                                            cellsWithDispersionShields.end(),
                                            cell));
    } else if (cell->physics.nearestDS) {
      //PHYS_CELL_DS_NEAREST_BIT: remove from list
      Cell* prev = cell->physics.prevDepDS, * next = cell->physics.nextDepDS;
      if (prev)
        prev->physics.nextDepDS = next;
      if (next)
        next->physics.prevDepDS = prev;
    }
  }

  //No sense bothering with the other inventories
  physicsClear(PHYS_SHIP_ENGINE_INVENTORY_BITS
              |PHYS_SHIP_SHIELD_INVENTORY_BITS
              |PHYS_SHIP_PBL_INVENTORY_BITS
              |PHYS_SHIP_WEAPON_INVENTORY_BITS
              );

  //Collision bounds are no longer valid
  collisionTree.free();

  //Done
}

void Ship::enableSoundEffects() noth {
  if (!soundEffects) {
    soundEffects = true;
    audio::ShipMixer::reset();
    audio_register(this);
    for (unsigned i=0; i<cells.size(); ++i)
      if (!cells[i]->isEmpty)
        audio::shipBackground.addSource(cells[i]);
    audio::ShipMixer::setShip(&cells[0], cells.size(), reinforcement);
  }
}

void Ship::disableSoundEffects() noth {
  if (soundEffects) {
    soundEffects = false;
    audio::ShipMixer::reset();
  }
}

bool damageBlameCompare(const Ship::damage_blame_t& a, const Ship::damage_blame_t& b) {
  return a.damage > b.damage;
}

const char* Ship::getDeathAttributions() noth {
  static char str[256];
  sort(damageBlame, damageBlame+lenof(damageBlame), damageBlameCompare);

  //If noone did more than 2 damage, suicide
  if (damageBlame[0].damage < 2) {
    sprintf(str, "%d", blame);
    return str;
  }

  //Assists and secondaries require the same minimum damage threshhold,
  //but the assist must be at least 1/10 the main
  if (damageBlame[1].damage*10 < damageBlame[0].damage || damageBlame[1].damage < 2) {
    sprintf(str, "%d", damageBlame[0].blame);
  //Secondary assists must do at least 1/4 the damage as main
  } else if (damageBlame[2].damage < 2) {
    sprintf(str, "%d %d", damageBlame[0].blame, damageBlame[1].blame);
  } else if (damageBlame[3].damage < 2) {
    sprintf(str, "%d %d %d", damageBlame[0].blame, damageBlame[1].blame, damageBlame[2].blame);
  } else {
    sprintf(str, "%d %d %d %d", damageBlame[0].blame, damageBlame[1].blame,
                                damageBlame[2].blame, damageBlame[3].blame);
  }
  return str;
}

float Ship::getDamageFrom(unsigned blame) const noth {
  for (unsigned i=0; i<lenof(damageBlame); ++i)
    if (damageBlame[i].blame == blame)
      return damageBlame[i].damage;
  return 0;
}

static const char* verify_impl(Ship* ship, bool phys) {
  if (ship->cells.size()==0) RETL10N(ship,err_empty)
  if (ship->cells.size() > MAX_CELL_CNT) RETL10N(ship,err_too_many_cells)
  if (typeid(*ship->cells[0]) == typeid(RightTCell)
  &&  ship->cells[0]->usage == CellBridge) RETL10N(ship,err_rt_bridge)

  for (unsigned i=0; i<ship->cells.size(); ++i) for (int s=0; s<2; ++s) {
    if (ship->cells[i]->systems[s] && ship->hasPower())
      if (const char* error=ship->cells[i]->systems[s]->acceptShip())
        return error;
    if (ship->cells[i]->systems[0] && ship->cells[i]->systems[1]) {
      ShipSystem* s0=ship->cells[i]->systems[0], *s1=ship->cells[i]->systems[1];
      if (s0->size==ShipSystem::Large && s1->size==ShipSystem::Large)
        RETL10N(ship,two_large_sys)
      if (s0->positioning==s1->positioning
      &&  s0->positioning != ShipSystem::Standard)
        RETL10N(ship,two_same_loc_sys)
    }

    if (typeid(*ship->cells[i])==typeid(CircleCell))
      for (int n=0; n<4; ++n)
        if (ship->cells[i]->neighbours[n] &&
            typeid(*ship->cells[i]->neighbours[n])==typeid(CircleCell))
          RETL10N(ship,adjacent_circles)
  }

  /* Test cells for overlap. We consider two cells to overlap if:
   * + Their collision rectangles, reduced to 95% size, collide,
   * + they are not neighbours, and
   * + they do not share a common neighbour.
   */
  for (unsigned i=1; i<ship->cells.size() && phys; ++i) for (unsigned j=0; j<i; ++j) {
    Cell* c=ship->cells[i], *d=ship->cells[j];
    float mdist;
    //Skip if Manhattan distance is greater than 2.5 cells
    mdist = fabs(c->getX()-d->getX()) + fabs(c->getY()-d->getY());
    if (mdist > STD_CELL_SZ*2.5f) goto next_j;
    //See if they are neighbours
    for (int n=0; n<4; ++n)
      if (c->neighbours[n]==d) goto next_j;
    //See if they have a common neighbour
    for (int nc=0; nc<4; ++nc) for (int nd=0; nd<4; ++nd)
      if (c->neighbours[nc] == d->neighbours[nd]) goto next_j;

    { //Put variables in different scope so the goto can skip them
      CollisionRectangle* ccrp=c->getCollisionBounds(), *dcrp=d->getCollisionBounds();
      if (ccrp && dcrp) {
        CollisionRectangle ccr=*ccrp, dcr=*dcrp;
        float cx=c->getX(), cy=c->getY(), dx=d->getX(), dy=d->getY();
        //Shrink each dimension of each to 0.85 the distance it originally was
        for (int n=0; n<4; ++n) {
          ccr.vertices[n].first  = cx + 0.85f*(ccr.vertices[n].first -cx);
          ccr.vertices[n].second = cy + 0.85f*(ccr.vertices[n].second-cy);
          dcr.vertices[n].first  = dx + 0.85f*(dcr.vertices[n].first -dx);
          dcr.vertices[n].second = dy + 0.85f*(dcr.vertices[n].second-dy);
        }
        //Do they collide?
        if (rectanglesCollide(ccr, dcr)) RETL10N(ship,cells_overlap)
      }
    } next_j:;
  }

  /* Verify sensicality of interconnections. All connections MUST pass the
   * same test used by the ship editor to make them.
   */
  vector<Cell*>& cells(ship->cells);
  for (unsigned h=0; h<cells.size() && phys; ++h) {
    Cell* cell=cells[h];
    for (unsigned cn=0; cn<4; ++cn)
    for (unsigned on=0; on<4; ++on) {
      if (cell->neighbours[cn]
      &&  cell->neighbours[cn]->neighbours[on] == cell) {
        Cell* neig = cell->neighbours[cn];
        //Check each edge point. If two of them are approx. equal, bind
        float cx=cell->getX(), cy=cell->getY(), ct=cell->getT()*pi/180.0f;
        float ox=neig->getX(), oy=neig->getY(), ot=neig->getT()*pi/180.0f;
        float ca=ct + cell->edgeT(cn)*pi/180.0f;
        if (ca>2*pi) ca-=2*pi;
        float oa=ot + neig->edgeT(on)*pi/180.0f;
        if (oa>2*pi) oa-=2*pi;
        float cd=cell->edgeD(cn);
        float od=neig->edgeD(on);
        float cex=cx + cd*cos(ca);
        float oex=ox + od*cos(oa);
        float cey=cy + cd*sin(ca);
        float oey=oy + od*sin(oa);

        float dx=cex-oex, dy=cey-oey;
        float dist=sqrt(dx*dx + dy*dy);
        if (dist>STD_CELL_SZ/32) {
          //Bad connection
          RETL10N(ship,invalid_cell_connection)
        }

        //Done with this neighbour
        break;
      }
    }
  }

  return NULL;
}

const char* verify(Ship* ship, bool phys) {
  float* restdat = ship->temporaryZero();
  const char* ret=verify_impl(ship, phys);
  ship->restoreFromZero(restdat);
  return ret;
}

