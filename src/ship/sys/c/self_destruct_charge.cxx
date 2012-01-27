/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/c/self_destruct_charge.hxx
 */

#include "self_destruct_charge.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/blast.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"

SelfDestructCharge::SelfDestructCharge(Ship* parent)
: ShipSystem(parent, system_texture::selfDestructCharge),
  detonated(false),
  willExplodeWhenDamaged((rand() % 100) == 0 && !parent->isRemote)
{
  _supportsStealthMode=true;
}

unsigned SelfDestructCharge::mass() const noth {
  return 10+(int)container->getMaxDamage();
}

void SelfDestructCharge::explode() noth {
  if (detonated) return;

  pair<float,float> coord = Ship::cellCoord(parent, container);
  if (!parent->isRemote)
    parent->getField()->inject(new Blast(parent->getField(), parent->blame, coord.first, coord.second,
                                         STD_CELL_SZ/2, //Fall off quickly
                                         container->getMaxDamage()*2, true, STD_CELL_SZ/2));
  detonated=true;

  //Explosion if appropriate
  if (EXPCLOSE(coord.first, coord.second)) {
    pair<float,float> vel = parent->getCellVelocity(container);
    Explosion ex(parent->getField(), Explosion::Incursion, 0.4f, 0.5f, 1.0f,
                 0.5f, 0.035f, 2000, //Size, density, lifetime
                 coord.first, coord.second,
                 vel.first, vel.second);
    parent->getField()->add(&ex);
  }
}

bool SelfDestructCharge::damage() noth {
  if (willExplodeWhenDamaged && !detonated) {
    explode();
    return false;
  } else {
    return !detonated;
  }
}

void SelfDestructCharge::destroy() noth {
  if (willExplodeWhenDamaged && !detonated) explode();
}

void SelfDestructCharge::selfDestruct() noth {
  ShipSystem::update = SelfDestructCharge::update;
  parent->refreshUpdates();
}

void SelfDestructCharge::update(ShipSystem* that, float) noth {
  ((SelfDestructCharge*)that)->explode();
}
