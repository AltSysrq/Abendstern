/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/reinforcement_bulkhead.hxx
 */

#include "src/ship/sys/ship_system.hxx"
#include "reinforcement_bulkhead.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"

ReinforcementBulkhead::ReinforcementBulkhead(Ship* s, bool stealth)
: ShipSystem(s, system_texture::reinforcementBulkhead), active(true) {
  _supportsStealthMode=stealth;
}

unsigned ReinforcementBulkhead::mass() const noth {
  return 10;
}

void ReinforcementBulkhead::shield_deactivate() noth {
  active=false;
  container->physicsClear(PHYS_CELL_REINFORCEMENT_BITS | PHYS_CELL_POWER_BITS);
}

float ReinforcementBulkhead::reinforcement_getAmt() noth {
  if (active && (!parent->isStealth() || supportsStealthMode()))
    return 2.5f;
  else
    return 1;
}

int ReinforcementBulkhead::normalPowerUse() const noth {
  return (active? 7 : 0);
}
