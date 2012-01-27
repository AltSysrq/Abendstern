/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/system_info_copier.hxx
 */

/*
 * system_info_copier.cxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#include <string>
#include <libconfig.h++>

#include "system_info_copier.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace std;
using namespace libconfig;

SystemInfoCopier::SystemInfoCopier(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void SystemInfoCopier::activate() {
  conf["edit"]["status_message"] = _(editor,system_info_copier_instructions);
}

void SystemInfoCopier::press(float x, float y) {
  Cell* target = manip->getCellAt(x,y);
  const Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"][target->cellName.c_str()]);
  if (s.exists("s0")) {
    conf["edit"]["sys0_type"] = (const char*)s["s0"]["type"];
    #define COPY_IFE(lname, rname, type) if (s["s0"].exists(rname)) \
      conf["edit"]["sys0_" lname] = (type)s["s0"][rname]
    COPY_IFE("capacitance", "capacity", int);
    COPY_IFE("shield_radius", "radius", float);
    COPY_IFE("shield_strength", "strength", float);
    COPY_IFE("gatling_turbo", "turbo", bool);
    #undef COPY_IFE
  } else conf["edit"]["sys0_type"] = "none";

  if (s.exists("s1")) {
    conf["edit"]["sys1_type"] = (const char*)s["s1"]["type"];
    #define COPY_IFE(lname, rname, type) if (s["s1"].exists(rname)) \
      conf["edit"]["sys1_" lname] = (type)s["s1"][rname]
    COPY_IFE("capacitance", "capacity", int);
    COPY_IFE("shield_radius", "radius", float);
    COPY_IFE("shield_strength", "strength", float);
    COPY_IFE("gatling_turbo", "turbo", bool);
    #undef COPY_IFE
  } else conf["edit"]["sys1_type"] = "none";

  conf["edit"]["current_mode"] = "none";
}

