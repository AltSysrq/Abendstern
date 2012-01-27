/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/system_creation_brush.hxx
 */

/*
 * system_creation_brush.cxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#include <set>
#include <string>

#include "system_creation_brush.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace std;

SystemCreationBrush::SystemCreationBrush(Manipulator* m, Ship*const& s)
: AbstractSystemPainter(m,s)
{ }

void SystemCreationBrush::motion(float x, float y, float, float) {
  if (!pressed) return;

  Cell* target=manip->getCellAt(x,y);
  if (paintedCells.find(target) == paintedCells.end()) {
    const char* err = paintSystems(target);
    paintedCells.insert(target);

    if (err) {
      static string msg;
      msg.clear();
      msg += "\a[(danger)";
      msg += err;
      msg += "\a]";
      conf["edit"]["status_message"] = msg.c_str();
    } else {
      conf["edit"]["status_message"] = _(editor,system_creation_brush_instructions);
    }

    ship->cellChanged(target);
  }
}

void SystemCreationBrush::activate() {
  AbstractSystemPainter::activate();
  conf["edit"]["status_message"] = _(editor,system_creation_brush_instructions);
}

void SystemCreationBrush::press(float x, float y) {
  AbstractSystemPainter::press(x,y);
  manip->pushUndo();
}

void SystemCreationBrush::release(float x, float y) {
  if (pressed) {
    if (const char* err = manip->reloadShip()) {
      static string msg;
      msg = "\a[(danger)";
      msg += err;
      msg += "\a]";
      conf["edit"]["status_message"] = msg.c_str();
      manip->popUndo();
      manip->reloadShip();
    } else {
      //Success
      manip->addToHistory();
    }
  }
  AbstractSystemPainter::release(x, y);
  paintedCells.clear();
}

