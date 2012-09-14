/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/manipulator.hxx
 */

/*
 * manipulator.cxx
 *
 *  Created on: 05.02.2011
 *      Author: jason
 */

#include <map>
#include <stack>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <exception>
#include <stdexcept>

#include <GL/gl.h>

#include <libconfig.h++>

#include "manipulator.hxx"
#include "manipulator_mode.hxx"
#include "cell_deletion_brush.hxx"
#include "cell_creation_brush.hxx"
#include "cell_rotator.hxx"
#include "system_creation_brush.hxx"
#include "system_creation_rectangle.hxx"
#include "system_info_copier.hxx"
#include "system_rotator.hxx"
#include "bridge_relocator.hxx"

#include "src/ship/ship.hxx"
#include "src/ship/shipio.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/sim/game_field.hxx"
#include "src/graphics/mat.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/asgi.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"

using namespace std;
using namespace libconfig;

Manipulator::Manipulator()
: currentMode(NULL), ship(NULL), field(new GameField(1,1)),
  panButtonDownCount(0)
{
  modes[string("none")]=NULL;
  modes[string("delete_cell")]=new CellDeletionBrush(this, ship);
  modes[string("create_cell")]=new CellCreationBrush(this, ship);
  modes[string("rotate_cell")]=new CellRotator(this, ship);
  modes[string("add_system_brush")]=new SystemCreationBrush(this, ship);
  modes[string("add_system_rect")] =new SystemCreationRectangle(this, ship);
  modes[string("system_info_copy")]=new SystemInfoCopier(this, ship);
  modes[string("rotate_system")]=new SystemRotator(this, ship);
  modes[string("move_bridge")]=new BridgeRelocator(this, ship);
}

Manipulator::~Manipulator() {
  if (ship) delete ship;
  delete field;
  for (map<string,ManipulatorMode*>::iterator it=modes.begin();
       it != modes.end(); ++it)
    delete (*it).second;
  while (!undoStates.empty()) {
    delete undoStates.front();
    undoStates.pop_front();
  }
  while (!historyStates.empty()) {
    delete historyStates.front();
    historyStates.pop_front();
  }
  glClearColor(0,0,0,1);
}

void Manipulator::update() {
  ManipulatorMode* switchTo = modes[string((const char*)conf["edit"]["current_mode"])];
  if (switchTo != currentMode) {
    if (currentMode) currentMode->deactivate();
    currentMode = switchTo;
    if (currentMode) currentMode->activate();
    else conf["edit"]["status_message"] = _(editor,idle);
  }
}

void Manipulator::draw() {
  if (ship) {
    glClearColor(0,0,0.05f,1);
    mPush();
    mConc(transform);
#ifdef AB_OPENGL_14
    matrix ttransform(~transform);
    glMultMatrixf(ttransform.v);
#endif
    ship->draw();
    if (currentMode) currentMode->draw();
    if (conf["edit"]["show_centre"]) {
      asgi::colour(1,1,1,1);
      asgi::begin(asgi::Lines);
      asgi::vertex(-10, vheight/2);
      asgi::vertex(+10, vheight/2);
      asgi::vertex(0.5f, -10);
      asgi::vertex(0.5f, +10);
      asgi::end();
    }
    mPop();
  } else glClearColor(0,0,0,1);
}

void Manipulator::primaryDown(float x, float y) {
  if (currentMode) {
    currentMode->press(x,y);
    currentMode->motion(x,y,0,0);
  }
}

void Manipulator::primaryUp(float x, float y) {
  if (currentMode) currentMode->release(x,y);
}

void Manipulator::secondaryDown(float,float) {
  ++panButtonDownCount;
}

void Manipulator::secondaryUp(float,float) {
  //Don't become negative ever
  if (panButtonDownCount) --panButtonDownCount;
}

void Manipulator::scrollUp(float x, float y) {
  //Zoom to cursor
  matrix trans1(1,0,0,x,
                0,1,0,y,
                0,0,1,0,
                0,0,0,1);
  matrix trans2(1,0,0,-x,
                0,1,0,-y,
                0,0,1,0,
                0,0,0,1);
  matrix zoom(1.25f,0,0,0,
              0,1.25f,0,0,
              0,0,1,0,
              0,0,0,1);
  transform = transform*trans2*zoom*trans1;
}

void Manipulator::scrollDown(float x, float y) {
  //Zoom to cursor
  matrix trans1(1,0,0,x,
                0,1,0,y,
                0,0,1,0,
                0,0,0,1);
  matrix trans2(1,0,0,-x,
                0,1,0,-y,
                0,0,1,0,
                0,0,0,1);
  matrix zoom(0.8f,0,0,0,
              0,0.8f,0,0,
              0,0,1,0,
              0,0,0,1);
  transform = transform*trans2*zoom*trans1;
}

void Manipulator::resetView() {
  matrix id;
  transform = id;
}

void Manipulator::motion(float cx, float cy, float rx, float ry) {
  if (panButtonDownCount)
    //Handle panning
    transform = transform *
                matrix(1,0,0,rx,
                       0,1,0,ry,
                       0,0,1,0,
                       0,0,0,1);

  //Forward to mode
  if (currentMode)
    currentMode->motion(cx,cy,rx,ry);
}

void Manipulator::pushUndo() {
  Config* config=new Config();
  const char* mountname = conf["edit"]["mountname"];
  Setting& src(conf[mountname]);
  confcpy(config->getRoot(), src);
  undoStates.push_back(config);
}

void Manipulator::popUndo() {
  if (undoStates.empty()) return;

  //First, clear current
  Setting& curr=conf[(const char*)conf["edit"]["mountname"]];
  while (curr.getLength()) curr.remove((unsigned)0);
  //Now, copy
  Config* undo=undoStates.back();
  confcpy(curr, undo->getRoot());

  //Romove old state
  delete undo;
  undoStates.pop_back();

  conf["edit"]["modified"]=true;
}

void Manipulator::commitUndo() {
  delete undoStates.back();
  undoStates.pop_back();
}

void Manipulator::activateMode() {
  if (currentMode) currentMode->activate();
}
void Manipulator::deactivateMode() {
  if (currentMode) currentMode->deactivate();
}

void Manipulator::addToHistory() {
  Config* config = new Config;
  const char* mountname = conf["edit"]["mountname"];
  Setting& src(conf[mountname]);
  confcpy(config->getRoot(), src);
  historyStates.push_back(config);

  conf["edit"]["history_times"].add(Setting::TypeInt) = SDL_GetTicks();
}

void Manipulator::revertToHistory(unsigned ix) {
  const char* mountname = conf["edit"]["mountname"];
  Setting& dst(conf[mountname]);
  while (dst.getLength()) dst.remove((unsigned)0);
  confcpy(dst, historyStates[ix]->getRoot());
  reloadShip();
  conf["edit"]["modified"]=true;
}

void Manipulator::deleteShip() {
  if (ship) delete ship;
  ship=NULL;
  for (unsigned i=0; i<undoStates.size(); ++i)
    delete undoStates[i];
  for (unsigned i=0; i<historyStates.size(); ++i)
    delete historyStates[i];
  undoStates.clear();
  historyStates.clear();
}

const char* Manipulator::reloadShip() {
  //A place to store error messages to return, since the exception will cease
  //to exist outside of a catch block
  static string message;

  if (ship) delete ship;
  ship=NULL; //In case the below fails
  try {
    //Delete signature if it exists
    string mount((const char*)conf["edit"]["mountname"]);
    if (conf[mount]["info"].exists("verification_signature"))
      conf[mount]["info"].remove("verification_signature");

    ship = loadShip(field, mount.c_str());
  } catch (NoSuchSettingException& nsse) {
    message = nsse.what();
    return message.c_str();
  } catch (runtime_error& err) {
    message = err.what();
    return message.c_str();
  } catch (...) {
    cerr << "Warning: In Manipulator::reloadShip(): Unknown error" << endl;
    return "Unknown error";
  }

  ship->physicsRequire(PHYS_SHIP_ALL | PHYS_CELL_ALL);

  ship->teleport(0.5f,vheight/2,0);

  ostringstream cat;
  cat << "cat" << (unsigned)ship->categorise();

  //Determine ship properties
  string mountname((const char*)conf["edit"]["mountname"]);
  conf["edit"]["ship_mass"]=ship->getMass();
  conf["edit"]["ship_reinforcement"] = ship->getReinforcement();
  conf["edit"]["ship_name"] = (const char*)conf[mountname]["info"]["name"];
  conf["edit"]["ship_class"] = (const char*)conf[mountname]["info"]["class"];
  conf["edit"]["ship_category"] =
    l10n::lookup('A', "editor", cat.str().c_str());
  if (conf[mountname]["info"].exists("author"))
    conf["edit"]["ship_author"] =
      (const char*)conf[mountname]["info"]["author"];
  else
    conf["edit"]["ship_author"] = "Anonymous";
  //Clear alliances, then copy
  while (conf["edit"]["ship_alliances"].getLength())
    conf["edit"]["ship_alliances"].remove((unsigned)0);
  confcpy(conf["edit"]["ship_alliances"], conf[mountname]["info"]["alliance"]);
  conf["edit"]["ship_radius"]=ship->getRadius()/STD_CELL_SZ;
  conf["edit"]["ship_cell_count"]=(int)ship->cells.size();
  float minX=999, minY=999, maxX=-999, maxY=-999;
  for (unsigned i=0; i<ship->cells.size(); ++i) {
    minX = min(minX, ship->cells[i]->getX());
    maxX = max(maxX, ship->cells[i]->getX());
    minY = min(minY, ship->cells[i]->getY());
    maxY = max(maxY, ship->cells[i]->getY());
  }
  conf["edit"]["ship_length"] = (maxX-minX)/STD_CELL_SZ;
  conf["edit"]["ship_width"] = (maxY-minY)/STD_CELL_SZ;
  //conf["edit"]["ship_cost"] = TODO;
  conf["edit"]["ship_power_gen"] = (int)ship->getPowerSupply();
  conf["edit"]["ship_power_use_no_accel"] = (int)ship->getPowerDrain();
  ship->configureEngines(true, false, 1.0f);
  conf["edit"]["ship_power_use_all_accel"] = (int)ship->getPowerDrain();
  conf["edit"]["ship_acceleration"] = ship->getAcceleration() / STD_CELL_SZ * 1000000;
  conf["edit"]["ship_rotaccel"] = ship->getRotationAccel() / pi * 180 * 1000000;


  //We define "supporting stealth mode" as:
  //Net power production, when accel on, brake off, 50% throttle,
  //stealth on, is non-negative AND acceleration is non-zero
  ship->configureEngines(true, false, 0.5f);
  ship->setStealthMode(true);
  conf["edit"]["ship_supports_stealth_mode"] = (
      ship->getPowerDrain() <= ship->getPowerSupply()
   && ship->getAcceleration() > 0);
  conf["edit"]["ship_power_gen_stealth"] = (int)ship->getPowerSupply();
  ship->configureEngines(false, false, 0);
  conf["edit"]["ship_power_use_stealth_no_accel"] = (int)ship->getPowerDrain();
  ship->configureEngines(true, false, 1);
  conf["edit"]["ship_power_use_stealth_all_accel"] = (int)ship->getPowerDrain();
  conf["edit"]["ship_stealth_acceleration"] = ship->getAcceleration() / STD_CELL_SZ * 1000000;
  conf["edit"]["ship_stealth_rotaccel"] = ship->getRotationAccel() / pi * 180 * 1000000;

  ship->configureEngines(false,false,0);
  ship->setStealthMode(false);

  if (conf["edit"]["show_shields"])
    shield_elucidate(ship);

  return NULL;
}

void Manipulator::screenToField(float& x, float& y) {
  //Undo the current transformation
  vec4 coord = Vec4(x,y,0,1);
  coord = coord * !transform;
  x = coord[0];
  y = coord[1];
}

Cell* Manipulator::getCellAt(float x, float y) {
  if (!ship) return NULL;

  screenToField(x,y);

  /* Determine which cell is the best match by summing
   * the distances from each vertex of each cell to the
   * cursor, including the dummy vertex of triangle cells.
   */
  Cell* nearest=NULL;
  float minDist = 65536;
  for (unsigned i=0; i<ship->cells.size(); ++i) {
    float sum=0;
    const CollisionRectangle& r(*ship->cells[i]->getCollisionBounds());
    for (unsigned n=0; n<4; ++n) {
      float dx = x - r.vertices[n].first;
      float dy = y - r.vertices[n].second;
      sum += dx*dx + dy*dy;
    }

    if (sum < minDist) {
      minDist = sum;
      nearest=ship->cells[i];
    }
  }

  return nearest;
}

void Manipulator::copyMounts(const char* dst, const char* src) {
  confcpy(conf[dst], conf[src]);
}
