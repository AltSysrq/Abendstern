/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/shipio.hxx
 */

/*
 * shipio.cxx
 *
 *  Created on: 18.07.2010
 *      Author: Jason Lingle
 */

#include <exception>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <libconfig.h++>
#include <map>

#include "everything.hxx"
#include "src/sim/game_field.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"

using namespace std;
using namespace libconfig;

static char cellName[5];
static vector<string> cellNames;
static map<string,unsigned> nameMap;

static void saveCell(Setting& cellRoot, Ship* ship, Cell* cell, char& clazz);
static void saveSystem(Setting& systemRoot, Ship* ship, Cell* cell, ShipSystem* sys, char& clazz);
void saveShip(Ship* ship, const char* mount) {
  Setting& root(conf[mount]);
  conf.modify(mount);
  //Remove existing data
  try { root.remove("cells"); } catch (...) {}
  try { root["info"].remove("version");
        root["info"].remove("bridge");
        root["info"].remove("class");
        root["info"].remove("reinforcement");
      } catch (...) {}

  char clazz[2] = "C";

  memset(cellName, 'A', 4);
  cellName[4]=0;
  cellNames.clear();
  //Generate all names now
  for (unsigned i=0; i<ship->cells.size(); ++i) {
    cellNames.push_back(string(cellName));
    if (cellName[0]=='Z') {
      cellName[0]='A';
      if (cellName[1]=='Z') {
        cellName[1]='A';
        if (cellName[2]=='Z') {
          cellName[2]='A';
          ++cellName[3];
        } else ++cellName[2];
      } else ++cellName[1];
    } else ++cellName[0];
  }

  try { root.add("info", Setting::TypeGroup); } catch (...) {}
  root.add("cells", Setting::TypeGroup);
  //Try to set name and alliance, but ignore failure (ie, if
  //they are already set)
  try {
    root["info"].add("name", Setting::TypeString)=mount;
    root["info"].add("alliance", Setting::TypeArray).add(Setting::TypeString)=mount;
  } catch (...) {}
  root["info"].add("version", Setting::TypeInt)=SHIPIO_VERSION;
  root["info"].add("bridge", Setting::TypeString)="AAAA";
  root["info"].add("reinforcement", Setting::TypeFloat)=ship->getReinforcement();
  for (unsigned i=0; i<ship->cells.size(); ++i) {
    saveCell(root["cells"].add(cellNames[i].c_str(), Setting::TypeGroup), ship, ship->cells[i], clazz[0]);
  }
  root["info"].add("class", Setting::TypeString)=clazz;
}

static void saveCell(Setting& root, Ship* ship, Cell* cell, char& clazz) {
  const char* cellType;
  const type_info& type=typeid(*cell);
  if      (type == typeid(SquareCell)) cellType="square";
  else if (type == typeid(CircleCell)) cellType="circle";
  else if (type == typeid(EquTCell  )) cellType="equil";
  else if (type == typeid(RightTCell)) cellType="right";
  else throw runtime_error("Unexpected cell type in ship passed to saveShip");
  root.add("type", Setting::TypeString)=cellType;
  Setting& ns=root.add("neighbours", Setting::TypeArray);
  for (unsigned i=0; i<4; ++i) {
    const char* name=NULL;
    for (unsigned j=0; !name && j<ship->cells.size(); ++j)
      if (cell->neighbours[i]==ship->cells[j])
        name=cellNames[j].c_str();
    if (cell->neighbours[i] && !name) {
      cerr << "FATAL: Should never get here!" << endl;
      exit(EXIT_PROGRAM_BUG);
    }
    ns.add(Setting::TypeString)=(cell->neighbours[i]? name : "");
  }
  if (cell->systems[0]) saveSystem(root.add("s0", Setting::TypeGroup), ship, cell, cell->systems[0], clazz);
  if (cell->systems[1]) saveSystem(root.add("s1", Setting::TypeGroup), ship, cell, cell->systems[1], clazz);
}

static void saveSystem(Setting& root, Ship* ship, Cell* cell, ShipSystem* sys, char& clazz) {
  const type_info& type=typeid(*sys);
  #define SYS(typ, cls) if (type==typeid(typ)) { \
    root.add("type", Setting::TypeString)=#typ; \
    if (cls < clazz) clazz=cls; \
  }
  //This will be different once groups exist
  #define WEA(typ, cls) if (type==typeid(typ)) { \
    root.add("type", Setting::TypeString)=#typ; \
    if (cls < clazz) clazz=cls; \
  }
       SYS(AntimatterPower, 'A')
  else SYS(BussardRamjet, 'B')
  else SYS(CloakingDevice, 'A')
  else SYS(DispersionShield, 'A')
  else WEA(EnergyChargeLauncher, 'C')
  else SYS(FissionPower, 'C')
  else SYS(FusionPower, 'B')
  else SYS(Heatsink, 'B')
  else WEA(MagnetoBombLauncher, 'C')
  else SYS(MiniGravwaveDrive, 'C')
  else SYS(MiniGravwaveDriveMKII, 'B')
  else WEA(MissileLauncher, 'A')
  else WEA(MonophasicEnergyEmitter, 'A')
  else SYS(ParticleAccelerator, 'C')
  else WEA(ParticleBeamLauncher, 'A')
  else WEA(PlasmaBurstLauncher, 'B')
  else SYS(PowerCell, 'C')
  else SYS(ReinforcementBulkhead, 'C')
  else SYS(RelIonAccelerator, 'A')
  else SYS(SelfDestructCharge, 'C')
  else WEA(SemiguidedBombLauncher, 'C')
  else SYS(SuperParticleAccelerator, 'B')
  else if (type==typeid(Capacitor)) {
    Capacitor* c=(Capacitor*)sys;
    root.add("capacity", Setting::TypeInt)=(int)c->getCapacity();
    root.add("type", Setting::TypeString)="Capacitor";
  } else if (type==typeid(ShieldGenerator)) {
    ShieldGenerator* g=(ShieldGenerator*)sys;
    root.add("type", Setting::TypeString)="ShieldGenerator";
    root.add("radius", Setting::TypeFloat)=g->getRadius();
    root.add("strength", Setting::TypeFloat)=g->getStrength();
  } else if (type==typeid(GatlingPlasmaBurstLauncher)) {
    root.add("type", Setting::TypeString)="GatlingPlasmaBurstLauncher";
    root.add("turbo", Setting::TypeBoolean)=((GatlingPlasmaBurstLauncher*)sys)->getTurbo();
  } else {
    cerr << "FATAL: Unknown ShipSystem type passed into saveSystem: " << type.name() << endl;
    throw runtime_error("Unknown system type");
  }

  if (sys->getOrientation() != -1) {
    root.add("orient", Setting::TypeInt)=sys->getOrientation();
  }
  #undef SYS
  #undef WEA
}

static Cell* loadCellInit(Setting&, Ship*, const char* clazz);
static void  loadCellLink(Setting&, Ship*, Cell*, const char* clazz);
static void  loadCellSystems(Setting&, Ship*, Cell*, const char* clazz);
static ShipSystem* loadSystem(Setting&, Ship*, Cell*, const char* clazz);
Ship* loadShip(GameField* field, const char* mount) {
  Ship* ship=new Ship(field);
  try {
    if (conf[mount]["info"]["version"].operator int() > SHIPIO_VERSION)
      throw runtime_error(_(ship,load_newer_version));
    /* Create a list of cell names, ensuring to put the
     * bridge at 0.
     */
    cellNames.clear();
    nameMap.clear();
    Setting& cellRoot=conf[mount]["cells"];
    bool foundBridge=false;
    cellNames.push_back(string((const char*)conf[mount]["info"]["bridge"]));
    nameMap[string((const char*)conf[mount]["info"]["bridge"])] = 0;
    for (unsigned i=0; i<cellRoot.getLength(); ++i) {
      const char* name=cellRoot[i].getName();
      if (cellNames[0]==name) foundBridge=true;
      else {
        cellNames.push_back(string(name));
        //If we haven't encountered the bridge yet, the index will
        //be one too low, since the bridge will be moved to index 0
        nameMap[string(name)] = (foundBridge? i : i+1);
      }
    }

    if (!foundBridge) throw runtime_error(_(ship,bridge_not_found));

    ship->cells.resize(cellNames.size());

    const char* clazz = conf[mount]["info"]["class"];
    if (strcmp(clazz, "C") && strcmp(clazz, "B") && strcmp(clazz, "A"))
      throw runtime_error("Unknown class");

    //Load all cells except for neighbour linkage and systems
    for (unsigned i=0; i<ship->cells.size(); ++i) {
      ship->cells[i]=loadCellInit(cellRoot[cellNames[i].c_str()], ship, clazz);
      ship->cells[i]->cellName=cellNames[i];
    }
    if (!cellRoot[cellNames[0].c_str()].exists("not_bridge") ||
        !cellRoot[cellNames[0].c_str()]["not_bridge"])
      ship->cells[0]->usage=CellBridge;
    //Link neighbours
    for (unsigned i=0; i<ship->cells.size(); ++i)
      loadCellLink(cellRoot[cellNames[i].c_str()], ship, ship->cells[i], clazz);
    ship->cells[0]->orient();
    //Load systems
    for (unsigned i=0; i<ship->cells.size(); ++i)
      loadCellSystems(cellRoot[cellNames[i].c_str()], ship, ship->cells[i], clazz);

    //Set default colour, if set
    try {
      float r, g, b;
      Setting& colour(conf[mount]["info"]["colour"]);
      r=colour[0], g=colour[1], b=colour[2];
      ship->setColour(r,g,b);
    } catch (...) { /* ignore errors here */ }

    //Set reinforcement
    ship->setReinforcement(conf[mount]["info"]["reinforcement"]);

    //Accept ship
    for (unsigned i=0; i<ship->cells.size(); ++i)
      for (unsigned s=0; s<2; ++s)
        if (ship->cells[i]->systems[s]) {
          ship->cells[i]->systems[s]->detectPhysics();
          if (const char* error=ship->cells[i]->systems[s]->acceptShip())
            throw runtime_error(error);
        }

    //Physics
    for (unsigned i=0; i<ship->cells.size(); ++i)
      for (unsigned s=0; s<2; ++s)
        if (ship->cells[i]->systems[s])
          ship->cells[i]->systems[s]->detectPhysics();

    /* We need to verify the validity of the ship.
     * This is an expensive process, and it does not
     * make sense to do it every time the ship is
     * reloaded. When a ship passes verification,
     * add a runtime signature to the info section that
     * allows this verification to be skipped.
     */
    static unsigned verificationSignature=rand();
    bool skipVerification=false;
    try {
      unsigned curr = (unsigned)conf[mount]["info"]["verification_signature"].operator int();
      skipVerification = curr == verificationSignature;
    } catch (...) { /* Ignore */ }

    //Verify if necessary
    if (!skipVerification) {
      ship->physicsRequire(PHYS_CELL_LOCATION_PROPERTIES_BIT);
      if (const char* error=verify(ship))
        throw runtime_error(error);
    }

    //OK, we passed verification. Sign the ship.
    //Delete current signature first, though
    if (conf[mount]["info"].exists("verification_signature"))
      conf[mount]["info"].remove("verification_signature");
    conf[mount]["info"].add("verification_signature", Setting::TypeInt) = (int)verificationSignature;

    //OK
    //Set mass. We don't have to set modified, because it doesn't
    //matter whether that value is saved to disk
    if (!conf[mount]["info"].exists("mass"))
      conf[mount]["info"].add("mass", Setting::TypeFloat);
    conf[mount]["info"]["mass"]=ship->getMass();

    ship->typeName=mount;
  } catch (...) {
    delete ship;
    throw;
  }

  return ship;
}

static Cell* loadCellInit(Setting& root, Ship* ship, const char* clazz) {
  const char* type=root["type"];
  #define T(str,typ) if (0==strcmp(type,#str)) return new typ(ship);
  T(square, SquareCell)
  T(circle, CircleCell)
  T(right,  RightTCell)
  T(equil,  EquTCell)
  throw runtime_error(_(ship,unknown_tell_type));
}

static void loadCellLink(Setting& root, Ship* ship, Cell* cell, const char*) {
  int len=root["neighbours"].getLength();
  if (len>4) throw runtime_error(_(ship,too_many_neighbours));
  for (int i=0; i<len; ++i) {
    const char* name=root["neighbours"][i];
    if (*name) {
      //Present
      //If i==3 and triangle, reject
      //We need to check this BEFORE orienting, since the generic code may try
      //to handle an invalid fourth neighbour
      if (i==3 && (0==strcmp("equil", root["type"]) || 0==strcmp("right", root["type"])))
        throw runtime_error(_(ship,triangular_4_neighbours));

      map<string,unsigned>::const_iterator it=nameMap.find(string(name));
      if (it==nameMap.end())
        throw runtime_error(_(ship,cell_ref_nx_neighbour));
      cell->neighbours[i]=ship->cells[it->second];
    }
  }
}

static void loadCellSystems(Setting& root, Ship* ship, Cell* cell, const char* clazz) {
  if (root.exists("s0")) cell->systems[0]=loadSystem(root["s0"], ship, cell, clazz);
  if (root.exists("s1")) cell->systems[1]=loadSystem(root["s1"], ship, cell, clazz);
}

static ShipSystem* loadSystem(Setting& root, Ship* ship, Cell* cell, const char* clazz) {
  const char* type=root["type"];
  ShipSystem* sys;
  #define SYS(typ,cls) if (0==strcmp(type,#typ)) { \
    if (clazz[0]>cls) throw runtime_error(_(ship,system_too_advanced)); \
    sys=new typ(ship); \
  }
  //Once weapons have groups, this will be different
  #define WEA(typ,cls) if (0==strcmp(type,#typ)) { \
    if (clazz[0]>cls) throw runtime_error(_(ship,system_too_advanced)); \
    sys=new typ(ship); \
  }

       SYS(AntimatterPower, 'A')
  else SYS(BussardRamjet, 'B')
  else SYS(CloakingDevice, 'A')
  else SYS(DispersionShield, 'A')
  else WEA(EnergyChargeLauncher, 'C')
  else SYS(FissionPower, 'C')
  else SYS(FusionPower, 'B')
  else SYS(Heatsink, 'B')
  else WEA(MagnetoBombLauncher, 'C')
  else SYS(MiniGravwaveDrive, 'C')
  else SYS(MiniGravwaveDriveMKII, 'B')
  else WEA(MissileLauncher, 'A')
  else WEA(MonophasicEnergyEmitter, 'A')
  else SYS(ParticleAccelerator, 'C')
  else WEA(ParticleBeamLauncher, 'A')
  else WEA(PlasmaBurstLauncher, 'B')
  else SYS(PowerCell, 'C')
  else SYS(ReinforcementBulkhead, 'C')
  else SYS(RelIonAccelerator, 'A')
  else SYS(SelfDestructCharge, 'C')
  else WEA(SemiguidedBombLauncher, 'B')
  else SYS(SuperParticleAccelerator, 'B')
  else if (0==strcmp(type, "Capacitor")) {
    sys=new Capacitor(ship, root["capacity"]);
  } else if (0==strcmp(type, "ShieldGenerator")) {
    if (clazz[0]>'B') throw runtime_error(_(ship,system_too_advanced));
    sys=new ShieldGenerator(ship, root["strength"], root["radius"]);
  } else if (0==strcmp(type, "GatlingPlasmaBurstLauncher")) {
    if (clazz[0]>'A') throw runtime_error(_(ship,system_too_advanced));
    sys=new GatlingPlasmaBurstLauncher(ship, root["turbo"]);
  } else throw runtime_error(_(ship,system_too_advanced));
  sys->container=cell;

  if (const char* error = (root.exists("orient")? sys->setOrientation(root["orient"]) : sys->autoOrient()))
    throw runtime_error(error);
  return sys;
  #undef WEA
  #undef SYS
}

