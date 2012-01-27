/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/ship_system.hxx
 */

#include <GL/gl.h>
#include <vector>

#include "ship_system.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/auxobj/shield.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/square.hxx"

using namespace std;

#ifdef AB_OPENGL_14
#define shipSystemTexture textureReplace
#endif

#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE shader::Texture
DELAY_SHADER(shipSystemTexture)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFORM(colourMap), NULL
END_DELAY_SHADER(static sysshader);

void ShipSystem::draw() const noth {
  UNIFORM_TYPE uni;
  uni.colourMap=0;

  glBindTexture(GL_TEXTURE_2D, texture);
  square_graphic::bind();
  sysshader->activate(&uni);
  square_graphic::draw();
}

void activateShipSystemShader() {
  UNIFORM_TYPE uni;
  uni.colourMap=0;
  sysshader->activate(&uni);
}

#define SFOREACH for (unsigned i=0, size=ship->cells.size(); i<size; ++i) \
                 for (unsigned s=0; s<2; ++s) if (ITEM)
#define SITEM ship->cells[i]->systems[s]
#define WFOREACH const vector<ShipSystem*>& wvec(ship->getWeapons((unsigned)clazz)); for (unsigned i=0; i<wvec.size(); ++i)
#define WITEM wvec[i]

#define INTPROTO(fun) FOREACH { int v=ITEM->fun(clazz); if (v!=-1) return v; } return -1
#define INTPROTOIMPL(fun) int fun(Ship* ship, Weapon clazz) noth { INTPROTO(fun); }
#define FLTPROTO(fun) FOREACH { float v=ITEM->fun(clazz); if (v==v) return v; } \
                      return NAN //float a=0.0f; return a/a
#define FLTPROTOIMPL(fun) float fun(Ship* ship, Weapon clazz) noth { FLTPROTO(fun); }
#define PTRPROTO(fun,typ) FOREACH { if (typ v=ITEM->fun(clazz)) return v; } return 0
#define PTRPROTOIMPL(fun,typ) typ fun(Ship* ship, Weapon clazz) noth { PTRPROTO(fun,typ); }
#define BOOLPROTO(fun) FOREACH if (ITEM->fun(clazz)) return true; /* end for */ return false
#define BOOLPROTOIMPL(fun) bool fun(Ship* ship, Weapon clazz) noth { BOOLPROTO(fun); }
#define VOIDPROTO(fun) FOREACH ITEM->fun(clazz)
#define VOIDPROTOIMPL(fun) void fun(Ship* ship, Weapon clazz) noth { VOIDPROTO(fun); }

//Weapons should used the W varieties
#define ITEM WITEM
#define FOREACH WFOREACH
INTPROTOIMPL(weapon_getEnergyLevel)
INTPROTOIMPL(weapon_getMinEnergyLevel)
INTPROTOIMPL(weapon_getMaxEnergyLevel)
PTRPROTOIMPL(weapon_getStatus, const char*)
PTRPROTOIMPL(weapon_getComment, const char*)
BOOLPROTOIMPL(weapon_isReady)
void weapon_setEnergyLevel(Ship* ship, Weapon clazz, int level) noth {
  FOREACH ITEM->weapon_setEnergyLevel(clazz, level);
}
void weapon_getWeaponInfo(Ship* ship, Weapon clazz, WeaponHUDInfo& info) noth {
  FOREACH if (ITEM->weapon_getWeaponInfo(clazz, info)) return;
}
FLTPROTOIMPL(weapon_getLaunchEnergy)
VOIDPROTOIMPL(weapon_incEnergyLevel)
VOIDPROTOIMPL(weapon_decEnergyLevel)
VOIDPROTOIMPL(weapon_fire)
void weapon_setTargettingMode(Ship* ship, Weapon clazz, TargettingMode mode) noth {
  FOREACH ITEM->weapon_setTargettingMode(clazz, mode);
}
TargettingMode weapon_getTargettingMode(Ship* ship, Weapon clazz) noth {
  FOREACH {
    TargettingMode m=ITEM->weapon_getTargettingMode(clazz);
    if (m != TargettingMode_NA) return m;
  }
  return TargettingMode_NA;
}
bool weapon_exists(Ship* ship, Weapon clazz) noth {
  FOREACH if (ITEM->clazz == Classification_Weapon && ITEM->weaponClass == clazz) return true;
  return false;
}

//Everything else does a full ship scan
#undef ITEM
#undef FOREACH
#define FOREACH SFOREACH
#define ITEM SITEM

void weapon_enumerate(Ship* ship, Weapon clazz, vector<ShipSystem*>& arg) {
  FOREACH ITEM->weapon_enumerate(clazz, arg);
}

void shield_enumerate(Ship* ship, vector<Shield*>& vec) noth {
  FOREACH ITEM->shield_enumerate(vec);
}
void disperser_enumerate(Ship* ship, vector<Cell*>& vec) noth {
  FOREACH ITEM->disperser_enumerate(vec);
}
void shield_deactivate(Ship* ship) noth {
  FOREACH ITEM->shield_deactivate();
}
void shield_elucidate(Ship* ship) noth {
  FOREACH ITEM->shield_elucidate();
}

void selfDestruct(Ship* ship) noth {
  FOREACH ITEM->selfDestruct();
}

float cooling_amount(Ship* ship)  noth{
  float total=0;
  FOREACH total+=ITEM->cooling_amount();
  return total;
}

int heating_count(Ship* ship) noth {
  int cnt=0;
  FOREACH if (ITEM->heating_count()) ++cnt;
  return cnt;
}

void audio_register(Ship* ship) noth {
  FOREACH ITEM->audio_register();
}
