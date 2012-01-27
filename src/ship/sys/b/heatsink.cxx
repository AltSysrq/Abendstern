/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/heatsink.hxx
 */

#include "src/ship/cell/cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/ship.hxx"
#include "heatsink.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"

Heatsink::Heatsink(Ship* par)
: ShipSystem(par, system_texture::heatsink)
{
}

signed Heatsink::normalPowerUse() const noth {
  return 25;
}
