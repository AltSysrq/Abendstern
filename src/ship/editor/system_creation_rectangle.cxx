/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/system_creation_rectangle.hxx
 */

/*
 * system_creation_rectangle.cxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#include <vector>
#include <string>
#include <cmath>

#include "system_creation_rectangle.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/sim/collision.hxx"
#include "src/graphics/asgi.hxx"
#include "src/core/lxn.hxx"

using namespace std;

SystemCreationRectangle::SystemCreationRectangle(Manipulator* m, Ship*const& s)
: AbstractSystemPainter(m,s)
{ }

void SystemCreationRectangle::press(float x, float y) {
  AbstractSystemPainter::press(x,y);
  manip->screenToField(x,y);
  cx=ox=x;
  cy=oy=y;
  targets.clear();
}

void SystemCreationRectangle::motion(float x, float y, float, float) {
  manip->screenToField(x,y);
  cx = x;
  cy = y;
  updateTargets();
}

void SystemCreationRectangle::release(float x,float y) {
  //Don't actually use these coordinates, as they might
  //not be correct (if the user uses keys to change the
  //current mode, or if released over a GUI component)

  if (pressed && !targets.empty()) {
    manip->pushUndo();
    for (unsigned i=0; i<targets.size(); ++i)
      paintSystems(targets[i]); //Ignore errors

    if (const char* err=manip->reloadShip()) {
      static string msg;
      msg = "\a[(danger)";
      msg += err;
      msg += "\a]";
      conf["edit"]["status_message"] = msg.c_str();
      manip->popUndo();
      manip->reloadShip();
    } else {
      conf["edit"]["status_message"] = _(editor,system_creation_rectangle_instructions);
      manip->addToHistory();
    }
  }

  AbstractSystemPainter::release(x,y);
  targets.clear();
}

void SystemCreationRectangle::activate() {
  conf["edit"]["status_message"] = _(editor,system_creation_rectangle_instructions);
  AbstractSystemPainter::activate();
}

void SystemCreationRectangle::draw() {
  if (!pressed) return;
  asgi::colour(0,1,0,0.5f);
  for (unsigned i=0; i<targets.size(); ++i) {
    const CollisionRectangle& r = *targets[i]->getCollisionBounds();
    asgi::begin(asgi::Quads);
    for (unsigned v=0; v<4; ++v)
      asgi::vertex(r.vertices[v].first, r.vertices[v].second);
    asgi::end();
  }

  asgi::colour(1,1,1,0.3f);
  asgi::begin(asgi::Quads);
  asgi::vertex(ox,oy);
  asgi::vertex(ox,cy);
  asgi::vertex(cx,cy);
  asgi::vertex(cx,oy);
  asgi::end();
}

void SystemCreationRectangle::updateTargets() {
  targets.clear();

  float minx = min(ox,cx), miny=min(oy,cy), maxx=max(ox,cx), maxy=max(oy,cy);
  for (unsigned i=0; i<ship->cells.size(); ++i) {
    const CollisionRectangle& r(*ship->cells[i]->getCollisionBounds());
    for (unsigned v=0; v<4; ++v) {
      if (r.vertices[v].first > minx && r.vertices[v].first < maxx
      &&  r.vertices[v].second > miny && r.vertices[v].second < maxy) {
        targets.push_back(ship->cells[i]);
        break;
      }
    }
  }
}
