/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/bridge_relocator.hxx
 */

/*
 * bridge_relocator.cxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#include <string>
#include <typeinfo>

#include <libconfig.h++>

#include "bridge_relocator.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/cell/square_cell.hxx"
#include "src/ship/cell/circle_cell.hxx"
#include "src/ship/cell/equt_cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace std;
using namespace libconfig;

BridgeRelocator::BridgeRelocator(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void BridgeRelocator::activate() {
  conf["edit"]["status_message"] = _(editor,bridge_relocator_instructions);
}

void BridgeRelocator::press(float x, float y) {
  Cell* target=manip->getCellAt(x, y);
  if (target->usage == CellBridge) {
    //Cancel
    conf["edit"]["current_mode"] = "none";
    return;
  }

  if (target->systems[0] || target->systems[1]) {
    conf["edit"]["status_message"] = _(editor,bridge_relocator_cell_not_empty);
    return;
  }

  int rotmul;
  if (typeid(*target) == typeid(SquareCell) || typeid(*target) == typeid(CircleCell))
    rotmul = 90;
  else if (typeid(*target) == typeid(EquTCell))
    rotmul = 120;
  else {
    conf["edit"]["status_message"] = _(editor,bridge_relocator_unsuitable_cell);
    return;
  }

  int theta=target->getT();
  while (theta < 0) theta += 360;
  if (theta % rotmul) {
    conf["edit"]["status_message"] = _(editor,bridge_relocator_cell_rotated);
    return;
  }

  int sides = target->numNeighbours();

  //Begin modifications
  manip->pushUndo();
  Setting& s(conf[(const char*)conf["edit"]["mountname"]]);
  Setting& n(s["cells"][target->cellName.c_str()]["neighbours"]);

  //Correct rotations
  int rotations = (360-theta)/rotmul % sides;
  for (int r=0; r<rotations; ++r) {
    string tmp((const char*)n[0]);
    n[0] = (const char*)n[1];
    n[1] = (const char*)n[2];
    if (sides == 3) {
      n[2] = tmp.c_str();
    } else {
      n[2] = (const char*)n[3];
      n[3] = tmp.c_str();
    }
  }

  //Change bridge name
  s["info"]["bridge"] = target->cellName.c_str();

  //Try to reload
  if (const char* err = manip->reloadShip()) {
    static string msg;
    msg = "\a[(danger)";
    msg += err;
    msg += "\a]";
    conf["edit"]["status_message"] = msg.c_str();

    manip->popUndo();
    manip->reloadShip();
  } else {
    conf["edit"]["modified"]=true;
    conf["edit"]["current_mode"]="none";

    manip->addToHistory();
  }
}
