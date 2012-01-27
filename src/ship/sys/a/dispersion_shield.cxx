/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/dispersion_shield.hxx
 */

/*
 * dispersion_shield.cxx
 *
 *  Created on: 13.02.2011
 *      Author: jason
 */

#include "dispersion_shield.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/ship/cell/cell.hxx"

DispersionShield::DispersionShield(Ship* parent)
: ShipSystem(parent, system_texture::dispersionShield, Classification_None, Standard, Large),
  isActive(true)
{ }

unsigned DispersionShield::mass() const noth {
  return 100;
}

signed DispersionShield::normalPowerUse() const noth {
  return 100;
}

void DispersionShield::disperser_enumerate(std::vector<Cell*>& v) noth {
  if (isActive) v.push_back(container);
}

void DispersionShield::shield_deactivate() noth {
  isActive=false;
  container->clearDSChain();
  container->physicsClear(PHYS_CELL_DS_NEAREST_BITS | PHYS_CELL_DS_EXIST_BITS);
  parent->physicsClear(PHYS_SHIP_DS_INVENTORY_BITS | PHYS_SHIP_POWER_BITS);
}
