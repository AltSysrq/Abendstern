/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/empty_cell.hxx
 */

#include <cmath>

#include "empty_cell.hxx"
#include "src/ship/auxobj/plasma_fire.hxx"

using namespace std;

EmptyCell::EmptyCell(Ship* ship, Cell* neighbour)
: Cell(ship), fire(NULL)
{
  _intrinsicMass=0;
  neighbours[0]=neighbour;
  x = neighbour->x;
  y = neighbour->y;
  netIndex=-1;
  isEmpty = true;

  //Fill in the physics
  physics.mass = 0;
  physics.angle = neighbour->physics.angle;
  physics.cosine = neighbour->physics.cosine;
  physics.sine = neighbour->physics.sine;
  physics.distance = neighbour->physics.distance;
  physics.thrustX = physics.thrustY = 0;
  physics.torque = 0;
  physics.torquePair = NULL;
  physics.rotationalThrust = 0;
  physics.cooling = 1;
  physics.powerUC = physics.powerUT = physics.powerSC = physics.powerST = 0;
  physics.ppowerU = physics.ppowerS = 0;
  physics.numHeaters = 0;
  physics.capacitance = 0;
  physics.hasDispersionShield = 0;
  physics.nearestDS = NULL;
  physics.nextDepDS = NULL;
  physics.distanceDS = -1;
  physics.reinforcement = 1;
  physics.valid = PHYS_CELL_ALL & ~PHYS_CELL_LOCATION_PROPERTIES_BIT;
}

EmptyCell::~EmptyCell() {
  if (fire) fire->fix=NULL;
}

unsigned EmptyCell::numNeighbours() const noth {
  return 1;
}

float EmptyCell::edgeD(int n) const noth {
  return 0;
}

int EmptyCell::edgeT(int n) const noth {
  return 0;
}

float EmptyCell::getIntrinsicDamage() const noth {
  return 0;
}

CollisionRectangle* EmptyCell::getCollisionBounds() noth {
  return NULL;
}

void EmptyCell::drawThis() noth {
}
void EmptyCell::drawShapeThis(float,float,float,float) noth {
}
void EmptyCell::drawDamageThis() noth {
}
