/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/weapon/plasma_burst.hxx
 */

#include <cmath>
#include <vector>
#include <typeinfo>
#include <cassert>

#include <GL/gl.h>

#include "plasma_burst.hxx"
#include "src/ship/auxobj/shield.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/collision.hxx"
#include "src/sim/blast.hxx"
#include "src/background/explosion.hxx"
#include "src/secondary/light_trail.hxx"
#include "src/globals.hxx"
//for STD_CELL_SZ
#include "src/ship/cell/cell.hxx"
#ifdef AB_OPENGL_14
#include "src/graphics/gl32emu.hxx"
#endif

using namespace std;

#define SPEED PB_SPEED
#define RADIUS PLASMA_BURST_RADIUS
#define DEGREDATION PB_DEGREDATION

PlasmaBurst::PlasmaBurst(GameField* field, Ship* par, float x, float y,
                         float svx, float svy, float theta, float initmass) :
  GameObject(field, x, y, svx+SPEED*cos(theta), svy+SPEED*sin(theta)),
  explodeListeners(NULL),
  parent(par),
  mass(initmass), direction(theta), timeUntilArm(50),
  inParentsShields(true), hitParentsShields(true), timeSinceLastExplosion(999),
  exploded(false), blame(par->blame)
{
  classification = GameObject::LightWeapon;
  colrect.radius=RADIUS*2;
  isExportable=true;
  collisionBounds.push_back(&colrect);
}

//Networking constructor
PlasmaBurst::PlasmaBurst(GameField* field, float x, float y,
                         float vx, float vy, float theta, float initmass)
: GameObject(field, x, y, vx, vy),
  explodeListeners(NULL), parent(NULL),
  mass(initmass), direction(theta), timeUntilArm(50),
  inParentsShields(true), hitParentsShields(true), timeSinceLastExplosion(999),
  exploded(false), blame(0xFFFFFF)
{
  classification = GameObject::LightWeapon;
  isExportable=true;
  isRemote=true;
  decorative=true;
  includeInCollisionDetection=false;
  collisionBounds.push_back(&colrect);
}

PlasmaBurst::~PlasmaBurst() {
  //Remove any ExplodeListener chain attached
  if (explodeListeners)
    explodeListeners->prv = NULL;
}

bool PlasmaBurst::update(float et) noth {
  if (timeUntilArm <= 0) {
    inParentsShields=hitParentsShields;
    hitParentsShields=false;
  }

  x+=vx*et;
  y+=vy*et;
  timeUntilArm-=et;
  /* When less than 1 remains, degrade linearly
   * into nothing after 1 sec.
   */
  if (mass>1) mass *= pow(DEGREDATION, et/1000);
  else        mass -= et/1000;

  if (isRemote) {
    REMOTE_XYCK;
    if (mass<0.001f) mass=0.001f;
  }

  #define VERT(i,xs,ys) colrect.vertices[i].first=x xs RADIUS; colrect.vertices[i].second=y ys RADIUS
  VERT(0,+,+);
  VERT(1,+,-);
  VERT(2,-,-);
  VERT(3,-,+);
  #undef VERT

  //Create an explosion for the trail for each physical frame
  if (!headless && currentVFrameLast &&
      (timeSinceLastExplosion+=currentFrameTime)>1 && EXPCLOSE(x,y)) {
    timeSinceLastExplosion=0;
    float evx = vx - SPEED*2/3*cos(direction),
          evy = vy - SPEED*2/3*sin(direction);

//    Explosion* ex = new Explosion(field, Explosion::Simple,
//                                  1.0f, 0.8f, 0.1f,
//                                  mass/25.0f, //sizeAt1Sec
//                                  (currentFrameTime>5? ((int)(currentFrameTime/5))*5 : 5)/5000.0f, //density
//                                  500, //lifetime
//                                  x, y, evx, evy,
//                                  //Smearing
//                                  SPEED*2/3*5000, pi+direction);
//    field->addBegin(ex);
    if (!trail.ref) {
      trail.assign(new LightTrail(field, 1000, 32, mass/12.5f, 0.005f,
                                  1.0f, 0.8f, 0.1f, 1.0f,
                                  0.0f, -1.3f, -1.5f, -2.0f));
      field->add(trail.ref);
    }
    ((LightTrail*)trail.ref)->setWidth(mass/12.5f);
    ((LightTrail*)trail.ref)->emit(x,y,evx,evy);
  }
  return mass>0;
}

void PlasmaBurst::draw() noth {
#ifdef AB_OPENGL_14
  glColor3f(1, mass/100.0f, mass/300.0f);
  gl32emu::setShaderEmulation(gl32emu::SE_quick);
  glBegin(GL_QUADS);
  for (unsigned i=0; i<4; ++i)
    glVertex2f(colrect.vertices[i].first, colrect.vertices[i].second);
  glEnd();
#endif /* AB_OPENGL_14 */
}

CollisionResult PlasmaBurst::checkCollision(GameObject* that) noth {
  if (timeUntilArm>0 || isRemote) return NoCollision;
  if (!that->isCollideable()) return UnlikelyCollision;
  if (objectsCollide(this, that)) return YesCollision;
  return UnlikelyCollision;
}

bool PlasmaBurst::collideWith(GameObject* other) noth {
  if (other==this) return false;
  if (!other->isCollideable()) return true;
  if (other==parent && inParentsShields) {
    hitParentsShields=true;
    return true;
  }

  explode(other);
  field->inject(new Blast(field, blame, x, y, STD_CELL_SZ/2, mass, true,
                          RADIUS*sqrt(2.0f)));
  return false;
}

void PlasmaBurst::explode(GameObject* other) noth {
  if (!other) other = this;

  if (EXPCLOSE(x,y)) {
    //Quantize mass to nearest 5 for caching
    float massQ=((int)((mass+5.0f)/5.0f))*5.0f;
    Explosion ex(field, Explosion::Spark, 1.0f, 0.8f, 0.1f,
                 mass*2/100, //sizeAt1Sec
                 massQ/200, //density
                 1000, //lifetime
                 x, y, other->getVX(), other->getVY());
    ex.multiExplosion(2);
  }

  exploded=true;
  //Reset velocities to that of other so that the velocity of the explosion
  //can be communicated over the network.
  vx = other->getVX();
  vy = other->getVY();

  for (ExplodeListener<PlasmaBurst>* l = explodeListeners; l; l = l->nxt)
    l->exploded(this);
}

float PlasmaBurst::getRadius() const noth {
  return RADIUS;
}
