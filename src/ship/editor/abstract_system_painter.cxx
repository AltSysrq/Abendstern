/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/editor/abstract_system_painter.hxx
 */

/*
 * abstract_system_painter.cxx
 *
 *  Created on: 08.02.2011
 *      Author: jason
 */

#include <cstring>
#include <cstdlib>
#include <typeinfo>
#include <string>

#include <libconfig.h++>

#include "abstract_system_painter.hxx"
#include "manipulator_mode.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/cell/square_cell.hxx"
#include "src/ship/cell/circle_cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/sys/a/antimatter_power.hxx"
#include "src/ship/sys/a/cloaking_device.hxx"
#include "src/ship/sys/a/dispersion_shield.hxx"
#include "src/ship/sys/a/gatling_plasma_burst_launcher.hxx"
#include "src/ship/sys/a/missile_launcher.hxx"
#include "src/ship/sys/a/monophasic_energy_emitter.hxx"
#include "src/ship/sys/a/particle_beam_launcher.hxx"
#include "src/ship/sys/a/rel_ion_accelerator.hxx"
#include "src/ship/sys/b/bussard_ramjet.hxx"
#include "src/ship/sys/b/fusion_power.hxx"
#include "src/ship/sys/b/heatsink.hxx"
#include "src/ship/sys/b/mini_gravwave_drive_mkii.hxx"
#include "src/ship/sys/b/plasma_burst_launcher.hxx"
#include "src/ship/sys/b/semiguided_bomb_launcher.hxx"
#include "src/ship/sys/b/shield_generator.hxx"
#include "src/ship/sys/b/super_particle_accelerator.hxx"
#include "src/ship/sys/c/capacitor.hxx"
#include "src/ship/sys/c/energy_charge_launcher.hxx"
#include "src/ship/sys/c/fission_power.hxx"
#include "src/ship/sys/c/magneto_bomb_launcher.hxx"
#include "src/ship/sys/c/mini_gravwave_drive.hxx"
#include "src/ship/sys/c/particle_accelerator.hxx"
#include "src/ship/sys/c/power_cell.hxx"
#include "src/ship/sys/c/reinforcement_bulkhead.hxx"
#include "src/ship/sys/c/self_destruct_charge.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"

using namespace libconfig;
using namespace std;

AbstractSystemPainter::AbstractSystemPainter(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void AbstractSystemPainter::deactivate() {
  if (pressed) release(0,0);
}

static ShipSystem* getSystem(const char* type, const string& prefix, Ship* ship) {
  string shstrength(prefix + "_shield_strength");
  string shradius(prefix + "_shield_radius");
  string capcap(prefix + "_capacitance");
  string gpblturbo(prefix + "_gatling_turbo");

  if (0 == strcmp(type, "delete"))
    return NULL;
  #define SYS(class, args) else if (0 == strcmp(type, #class)) return new class args
  SYS(AntimatterPower, (ship));
  SYS(CloakingDevice, (ship));
  SYS(DispersionShield, (ship));
  SYS(GatlingPlasmaBurstLauncher, (ship, conf["edit"][gpblturbo.c_str()]));
  SYS(MissileLauncher, (ship));
  SYS(MonophasicEnergyEmitter, (ship));
  SYS(ParticleBeamLauncher, (ship));
  SYS(RelIonAccelerator, (ship));
  SYS(BussardRamjet, (ship));
  SYS(FusionPower, (ship));
  SYS(Heatsink, (ship));
  SYS(MiniGravwaveDriveMKII, (ship));
  SYS(PlasmaBurstLauncher, (ship));
  SYS(SemiguidedBombLauncher, (ship));
  SYS(ShieldGenerator, (ship, conf["edit"][shstrength.c_str()], conf["edit"][shradius.c_str()]));
  SYS(SuperParticleAccelerator, (ship));
  SYS(Capacitor, (ship, conf["edit"][capcap.c_str()]));
  SYS(EnergyChargeLauncher, (ship));
  SYS(FissionPower, (ship));
  SYS(MagnetoBombLauncher, (ship));
  SYS(MiniGravwaveDrive, (ship));
  SYS(ParticleAccelerator, (ship));
  SYS(PowerCell, (ship));
  SYS(ReinforcementBulkhead, (ship));
  SYS(SelfDestructCharge, (ship));
  else {
    cerr << "FATAL: Unknown system type: " << type << endl;
    exit(EXIT_SCRIPTING_BUG);
  }
}

/* This function exists to pacify G++. If I try something like:
 *   if (x && e="foo")
 * I get "assignment requires lvalue". This expands to exactly
 * the same code, yet it works.
 */
static inline const char* asgn(const char*& l, const char* r) {
  l=r;
  return l;
}

const char* AbstractSystemPainter::paintSystems(Cell* cell) {
  if (cell->usage == CellBridge) return _(editor,cant_put_systems_in_bridge);

  const char* error=NULL;

  Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"][cell->cellName.c_str()]);
  const char* ns0type = conf["edit"]["sys0_type"], *ns1type = conf["edit"]["sys1_type"];
  bool hasSys1 = (typeid(*cell) == typeid(SquareCell) || typeid(*cell) == typeid(CircleCell));

  bool sys0Mod=true, sys1Mod=true;

  //Begin by backing current systems up
  //Delete these if non-NULL at end of function
  ShipSystem* s0 = cell->systems[0], * s1 = cell->systems[1];

  if (0 == strcmp(ns0type, "none")) {
    //No change, don't delete current
    s0=NULL;
    sys0Mod=false;
  } else {
    cell->systems[0] = getSystem(ns0type, string("sys0"), ship);
    if (cell->systems[0]) cell->systems[0]->container = cell;
    const char* e=NULL;
    if (cell->systems[0]
    &&  (asgn(e,cell->systems[0]->autoOrient()) || asgn(e,cell->systems[0]->acceptShip()))) {
      if (!error) error=e;
      //Restore back-up
      delete cell->systems[0];
      cell->systems[0]=s0;
      s0=NULL;
      sys0Mod=false;
    }
  }

  if (!hasSys1 || 0 == strcmp(ns1type, "none")) {
    //No change
    s1=NULL;
    sys1Mod=false;
  } else {
    cell->systems[1] = getSystem(ns1type, string("sys1"), ship);
    if (cell->systems[1]) cell->systems[1]->container = cell;
    const char* e=NULL;
    if (cell->systems[1]
    && (asgn(e,cell->systems[1]->autoOrient()) || asgn(e,cell->systems[1]->acceptShip()))) {
      if (!error) error=e;
      delete cell->systems[1];
      cell->systems[1]=s1;
      s1=NULL;
      sys1Mod=false;
    }
  }

  /* If the ship still verifies, the result is still OK;
   * otherwise, we need to undo everything.
   */
  if (const char* e = verify(ship, false)) {
    if (!error) error=e;
    if (sys0Mod) {
      if (cell->systems[0]) delete cell->systems[0];
      cell->systems[0] = s0;
      s0=NULL;
      sys0Mod=false;
    }
    if (sys1Mod) {
      if (cell->systems[1]) delete cell->systems[1];
      cell->systems[1] = s1;
      s1=NULL;
      sys1Mod=false;
    }
  }

  //Apply changes to configuration
  if (sys0Mod) {
    if (s.exists("s0"))
      s.remove("s0");
    if (cell->systems[0]) {
      Setting& sys(s.add("s0", Setting::TypeGroup));
      sys.add("type", Setting::TypeString) = ns0type;
      if (cell->systems[0]->getOrientation() != -1)
        sys.add("orient", Setting::TypeInt) = cell->systems[0]->getOrientation();
      if (0 == strcmp(ns0type, "Capacitor"))
        sys.add("capacity", Setting::TypeInt) = (int)conf["edit"]["sys0_capacitance"];
      if (0 == strcmp(ns0type, "ShieldGenerator")) {
        sys.add("radius", Setting::TypeFloat) = (float)conf["edit"]["sys0_shield_radius"];
        sys.add("strength", Setting::TypeFloat) = (float)conf["edit"]["sys0_shield_strength"];
      }
      if (0 == strcmp(ns0type, "GatlingPlasmaBurstLauncher")) {
        sys.add("turbo", Setting::TypeBoolean) = (bool)conf["edit"]["sys0_gatling_turbo"];
      }
    }
  }

  if (sys1Mod) {
    if (s.exists("s1"))
      s.remove("s1");
    if (cell->systems[1]) {
      Setting& sys(s.add("s1", Setting::TypeGroup));
      sys.add("type", Setting::TypeString) = ns1type;
      if (cell->systems[1]->getOrientation() != -1)
        sys.add("orient", Setting::TypeInt) = cell->systems[1]->getOrientation();
      if (0 == strcmp(ns1type, "Capacitor"))
        sys.add("capacity", Setting::TypeInt) = (int)conf["edit"]["sys1_capacitance"];
      if (0 == strcmp(ns1type, "ShieldGenerator")) {
        sys.add("radius", Setting::TypeFloat) = (float)conf["edit"]["sys1_shield_radius"];
        sys.add("strength", Setting::TypeFloat) = (float)conf["edit"]["sys1_shield_strength"];
      }
      if (0 == strcmp(ns1type, "GatlingPlasmaBurstLauncher")) {
        sys.add("turbo", Setting::TypeBoolean) = (bool)conf["edit"]["sys1_gatling_turbo"];
      }
    }
  }

  if (sys0Mod || sys1Mod)
    conf["edit"]["modified"] = true;

  //Delete backups
  if (s0) delete s0;
  if (s1) delete s1;
  //Forse re-inventory of ship shields (and other infos)
  ship->physicsClear(PHYS_SHIP_ALL | PHYS_CELL_ALL);

  return error;
}
