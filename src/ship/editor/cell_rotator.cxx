/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/cell_rotator.hxx
 */

/*
 * cell_rotator.cxx
 *
 *  Created on: 08.02.2011
 *      Author: jason
 */
#include <string>
#include <cstring>

#include "cell_rotator.hxx"
#include "manipulator.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace std;

CellRotator::CellRotator(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void CellRotator::activate() {
  conf["edit"]["status_message"] = _(editor,cell_rotator_instructions);
}

void CellRotator::press(float x, float y) {
  Cell* cell = manip->getCellAt(x,y);
  libconfig::Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"][cell->cellName.c_str()]);
  if (strcmp(s["type"], "right")) return;

  //Don't save any change that results in the cell being in the same place
  unsigned rots=0;
  manip->pushUndo();
  do {
    ++rots;
    string tmp((const char*)s["neighbours"][0]);
    s["neighbours"][0] = (const char*)s["neighbours"][1];
    s["neighbours"][1] = (const char*)s["neighbours"][2];
    s["neighbours"][2] = tmp.c_str();
  } while (manip->reloadShip());

  if (rots == 3) manip->commitUndo(); //No effect
  else manip->addToHistory(); //Success
}
