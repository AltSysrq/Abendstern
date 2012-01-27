/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_state/aim_edge_detect.hxx
 */

/*
 * aim_edge_detect.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>
#include <string>

#include "aim_edge_detect.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/game_field.hxx"

AIM_EdgeDetect::AIM_EdgeDetect(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  projection(s.exists("consider_this_scary")? s["consider_this_scary"] : 10000),
  scaredName((const char*)s["scared"]),
  otherName(s.exists("otherwise")? (const char*)s["otherwise"] : "")
{ }

void AIM_EdgeDetect::action() {
  float fx = ship->getX() + projection*ship->getVX();
  float fy = ship->getY() + projection*ship->getVY();
  if (fx < 0 || fy < 0 || fx > ship->getField()->width || fy > ship->getField()->height)
    //Scared!
    controller.setState(scaredName.c_str());
  else
    //Calm
    controller.setState(otherName.c_str());
}

static AIModuleRegistrar<AIM_EdgeDetect> registrar("state/edge_detect");
