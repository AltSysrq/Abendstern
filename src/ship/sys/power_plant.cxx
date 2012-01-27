/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/power_plant.hxx
 */

#include <cstdlib>
#include <iostream>

#include "power_plant.hxx"
#include "src/exit_conditions.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"

using namespace std;

bool PowerPlant::damage(unsigned blame) noth {
  //Do nothing if already damaged or if a remote system
  if (ShipSystem::update || parent->isRemote) return true;

  timeUntilExplosion = rand()/(float)RAND_MAX * 1000;
  explosionBlame = blame;
  ShipSystem::update = PowerPlant::update;
  //Ensure the update method is added by the ship
  container->parent->refreshUpdates();
  return true;
}

void PowerPlant::update(ShipSystem* thatSS, float et) noth {
  PowerPlant* that=(PowerPlant*)thatSS;
  if ((that->timeUntilExplosion -= et) <= 0) {
    if      (that->container->systems[0] == that) that->container->systems[0]=NULL;
    else if (that->container->systems[1] == that) that->container->systems[1]=NULL;
    else {
      cerr << "FATAL: PowerPlant unable to find self in container" << endl;
      exit(EXIT_THE_SKY_IS_FALLING);
    }
    //Ensure that the parent notices the system deletion
    that->container->parent->refreshUpdates();
    that->container->physicsClear(PHYS_CELL_POWER_BITS
                                 |PHYS_CELL_POWER_PROD_BITS
                                 |PHYS_CELL_MASS_BITS);
    that->destroy(that->explosionBlame);
    delete that;
  }
}
