/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/b/shield_generator.hxx
 */

#include <cmath>
#include <iostream>

#include "shield_generator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/ship/sys/system_textures.hxx"
#include "src/audio/ship_effects.hxx"

using namespace std;

//unit/r/dmg
#define POWER_USAGE 0.5f
#define MASS 4

ShieldGenerator::ShieldGenerator(Ship* ship, float strength, float radius)
: ShipSystem(ship, system_texture::shieldGenerator, Classification_Shield, ShipSystem::Standard, ShipSystem::Large),
  shield(NULL), _strength(strength), _radius(radius),
  deathPoint(-1), dead(false)
{
}

void ShieldGenerator::detectPhysics() noth {
  if (shield) {
    shield->parent=container;
    //force it to recalculate distance
    shield->updateDist();
  } else if (!dead && parent->hasPower()) {
    shield=new Shield(parent->getField(), container, _strength, _radius);
    shield->updateDist(); //Force distance recalc
  }
}

unsigned ShieldGenerator::mass() const noth {
  return _radius*_radius*MASS;
}

signed ShieldGenerator::normalPowerUse() const noth {
  return (signed)(dead? 0.0f : POWER_USAGE*_strength*_radius);
}

void ShieldGenerator::shield_enumerate(vector<Shield*>& vec) noth {
  if (shield && (!parent->isStealth() || supportsStealthMode()))
    vec.push_back(shield);
}

void ShieldGenerator::shield_deactivate() noth {
  dead=true;
  if (shield) delete shield; //shield->parent=NULL;
  shield=NULL;
  container->physicsClear(PHYS_CELL_POWER_BITS);
  parent->physicsClear(PHYS_SHIP_SHIELD_INVENTORY_BITS);
}

void ShieldGenerator::shield_elucidate() noth {
  shield->alpha=0.4f;
}

void ShieldGenerator::write(ostream& out) noth {
  out << _strength << ' ' << _radius << ' ';
}

void ShieldGenerator::audio_register() const noth {
  audio::shieldGenerator.addSource(container);
}

ShipSystem* ShieldGenerator::clone() noth {
  return new ShieldGenerator(parent, _strength, _radius);
}

float ShieldGenerator::getShieldStability() const noth {
  return (shield? shield->stability : 1.0f);
}
float ShieldGenerator::getShieldStrength() const noth {
  return (shield? shield->strength/shield->maxStrength : 1.0f);
}
float ShieldGenerator::getShieldAlpha() const noth {
  return (shield? shield->alpha : 1.0f);
}
void ShieldGenerator::setShieldStability(float f) noth {
  if (shield) shield->stability=f;
}
void ShieldGenerator::setShieldStrength(float f) noth {
  if (shield) shield->strength=f*shield->maxStrength;
}
void ShieldGenerator::setShieldAlpha(float f) noth {
  if (shield) shield->alpha=f;
}
