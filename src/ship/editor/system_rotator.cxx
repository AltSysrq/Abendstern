/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/system_rotator.hxx
 */

/*
 * system_rotator.cxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#include <string>

#include <libconfig.h++>

#include "system_rotator.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/core/lxn.hxx"

using namespace libconfig;

SystemRotator::SystemRotator(Manipulator* m, Ship*const& s)
: ManipulatorMode(m,s)
{ }

void SystemRotator::activate() {
  conf["edit"]["status_message"] = _(editor,system_rotator_instructions);
}

void SystemRotator::press(float x, float y) {
  Cell* target=manip->getCellAt(x,y);
  if (target->cellName != prevTarget) {
    prevTarget = target->cellName;
    rotCount = 0;
  }

  Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"][target->cellName.c_str()]);
  Setting* s0 = NULL, *s1 = NULL;
  if (s.exists("s0") && s["s0"].exists("orient") && ((int)s["s0"]["orient"]) != -1)
    s0 = &s["s0"]["orient"];
  if (s.exists("s1") && s["s1"].exists("orient") && ((int)s["s1"]["orient"]) != -1)
    s1 = &s["s1"]["orient"];

  if (!s0 && !s1) return; //Nothing to rotate

  conf["edit"]["modified"]=true;

  //The code below is simpler if we make it look like there are
  //two systems
  if (!s0) s0=s1;
  if (!s1) s1=s0;

  int s0o = *s0, s1o = *s1;

  Setting* curr = (rotCount < 4? s0 : s1);
  manip->pushUndo();
  while (true) {
    (*curr) = (curr->operator int() + 1)%4;
    ++rotCount;
    rotCount &= 0x7;

    if (!manip->reloadShip()) {
      //OK
      if (rotCount) manip->addToHistory();
      return;
    }

    //This rotation is invalid, try the next.
    //If we are changing systems, though, restore
    //original for this
    if (rotCount == 0 || rotCount == 4)
      (*curr) = (rotCount == 0? s1o : s0o);
  }
}
