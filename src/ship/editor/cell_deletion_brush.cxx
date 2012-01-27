/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/cell_deletion_brush.hxx
 */

/*
 * cell_deletion_brush.cxx
 *
 *  Created on: 06.02.2011
 *      Author: jason
 */

#include <vector>
#include <algorithm>
#include <libconfig.h++>
#include <iostream>

#include "cell_deletion_brush.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace std;

CellDeletionBrush::CellDeletionBrush(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void CellDeletionBrush::activate() {
  conf["edit"]["status_message"] = _(editor,cell_deletion_brush_instructions);
}

void CellDeletionBrush::press(float x, float y) {
  Cell* target=manip->getCellAt(x,y);

  if (target == ship->cells[0]) {
    conf["edit"]["status_message"] = _(editor,cell_deletion_brush_bridge);
    return;
  }

  vector<Cell*> toDelete;
  toDelete.push_back(target);
  //Remove from ship and see what's orphaned
  ship->removeCell(target);
  for (unsigned n=0; n<4; ++n)
    if (target->neighbours[n])
      for (unsigned m=0; m<4; ++m)
        if (target->neighbours[n]->neighbours[m] == target)
          target->neighbours[n]->neighbours[m] = NULL;
  vector<Cell*> nonOrphans;
  ship->cells[0]->getAdjoined(nonOrphans);
  //Add any Ship cells into the toDelete vector and remove from
  //the Ship. Then, remove their config entries and delete them.
  for (unsigned i=0; i<ship->cells.size(); ++i)
    if (nonOrphans.end() == find(nonOrphans.begin(), nonOrphans.end(), ship->cells[i])) {
      toDelete.push_back(ship->cells[i]);
      ship->cells.erase(ship->cells.begin() + (i--));
    }

  manip->pushUndo();
  libconfig::Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"]);
  for (unsigned i=0; i<toDelete.size(); ++i) {
    //Remove references to the cell
    for (unsigned j=0; j<(unsigned)s.getLength(); ++j)
      for (unsigned n=0; n<(unsigned)s[j]["neighbours"].getLength(); ++n)
        if (toDelete[i]->cellName == (const char*)s[j]["neighbours"][n])
          s[j]["neighbours"][n] = "";
    s.remove(toDelete[i]->cellName.c_str());
    delete toDelete[i];
  }

  //See if the change was valid
  if (const char* err = manip->reloadShip()) {
    static string msg;
    msg = "\a[(danger)";
    msg += err;
    msg += "\a]";
    //Nope
    manip->popUndo();
    manip->reloadShip();
    conf["edit"]["status_message"] = msg.c_str();
  } else {
    conf["edit"]["status_message"] = "Delete cells";
    conf["edit"]["modified"] = true;
    manip->addToHistory();
  }
}
