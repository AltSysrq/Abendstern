/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/editor/cell_creation_brush.hxx
 */

/*
 * cell_creation_brush.cxx
 *
 *  Created on: 07.02.2011
 *      Author: jason
 */

#ifndef DEBUG
#define NDEBUG
#endif /* !DEBUG */

#include <cmath>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <iostream>

#include <libconfig.h++>

#include "cell_creation_brush.hxx"
#include "manipulator.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/cell/square_cell.hxx"
#include "src/ship/cell/circle_cell.hxx"
#include "src/ship/cell/equt_cell.hxx"
#include "src/ship/cell/rightt_cell.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"
#include "src/exit_conditions.hxx"
#include "src/sim/collision.hxx"
#include "src/graphics/asgi.hxx"
#include "src/core/lxn.hxx"

using namespace std;

/* Consonant/vowel pairs used for forming new cell names.
 * The idea here is to form Japanese-like "words".
 */
static const char*const consonants[] = {
  "b", "d", "f", "g", "h", "j", "k", "m", "n", "p", "r",
  "ry", "s", "sh", "t", "w", "y",
};
static const char*const vowels[] = {
  "a", "ai", "e", "ei", "i", "o", "ou", "oi", "u"
};

CellCreationBrush::CellCreationBrush(Manipulator* man, Ship*const& s)
: ManipulatorMode(man,s), isActive(false)
{ }

void CellCreationBrush::activate() {
  conf["edit"]["status_message"] = _(editor,cell_creation_brush_instructions);
}

void CellCreationBrush::deactivate() {
  exitActiveMode(NULL);
  release(0,0);
}

void CellCreationBrush::motion(float x, float y, float,float) {
  if (!pressed) return;

  Cell* target = manip->getCellAt(x,y);
  if (!target) return;

  if (isActive) {
    //Do nothing for motion in active cell
    if (target->cellName == activeCell) return;
    else exitActiveMode(target);
  } else {
    if (!enterActiveMode(target))
      conf["edit"]["status_message"] = _(editor,cell_creation_brush_error);
  }
}

void CellCreationBrush::press(float x,float y) {
  ManipulatorMode::press(x,y);
  modificationsDuringActivation=false;
  manip->pushUndo();
}

void CellCreationBrush::release(float x, float y) {
  if (pressed) {
    exitActiveMode(NULL);
    if (!modificationsDuringActivation)
      manip->commitUndo(); //Forget useless undo state
    else {
      const char* err=manip->reloadShip();
      if (err) {
        static string msg;
        msg.clear();
        msg += "\a[(danger)";
        msg += err;
        msg += "\a]";
        cout << msg.c_str() << endl;
        //Can't paint
        manip->popUndo();
        manip->reloadShip();
        conf["edit"]["status_message"] = msg.c_str();
      } else {
        //Success, add to history
        manip->addToHistory();
      }
    }
  }
  ManipulatorMode::release(x,y);
  paintCells.clear();
}

void CellCreationBrush::draw() {
  /* ASGI isn't terribly fast, but it's the most practical
   * way to handle this.
   */
  for (unsigned i=0; i<paintCells.size(); ++i) {
    asgi::begin(asgi::Quads);
    bool isGhost = false;
    for (unsigned n=0; n<4; ++n)
      if (temporaries[n] == paintCells[i]->cellName)
        isGhost=true;

    if (isGhost)
      asgi::colour(0,1,1,0.8f);
    else
      asgi::colour(1,0.5f,0.1f,0.8f);

    const CollisionRectangle& r(*paintCells[i]->getCollisionBounds());
    for (unsigned v=0; v<4; ++v)
      asgi::vertex(r.vertices[v].first, r.vertices[v].second);
    asgi::end();
  }

  for (unsigned i=0; i<whyNotCells.size(); ++i) {
    asgi::begin(asgi::Quads);
    asgi::colour(1,0,0,0.8);

    CollisionRectangle r1(*whyNotCells[i]->getCollisionBounds());
    float tx=0, ty=0;
    for (unsigned j=0; j<4; ++j) {
      tx += r1.vertices[j].first;
      ty += r1.vertices[j].second;
    }
    float cx = tx/4, cy=ty/4;
    for (unsigned j=0; j<4; ++j) {
      r1.vertices[j].first = cx + (r1.vertices[j].first - cx)*0.8f;
      r1.vertices[j].second= cy + (r1.vertices[j].second- cy)*0.8f;
    }
    for (unsigned v=0; v<4; ++v)
      asgi::vertex(r1.vertices[v].first, r1.vertices[v].second);
    asgi::end();
  }
}


bool CellCreationBrush::enterActiveMode(Cell* cell) {
  manip->pushUndo();
  whyNotCells.clear();

  //Clear all temporary names
  for (unsigned i=0; i<4; ++i)
    temporaries[i] = "";

  bool entered=false;
  activeCell = cell->cellName;
  unsigned numTemps = cell->numNeighbours();
  for (unsigned i=0; i<numTemps; ++i)
    entered |= spawnTemporary(i);

  //Our handle on cell is not valid anymore

  if (entered) {
    isActive=true;
    return true;
  } else {
    //No new neighbours here
    manip->commitUndo(); //Forget undo state
    return false;
  }
}

void CellCreationBrush::exitActiveMode(Cell* cell) {
  whyNotCells.clear();
  if (!isActive) return;

  isActive=false;

  //See if the cell represents any of the temporaries
  //If so, set modified and make that no longer a temporary
  if (cell) {
    for (unsigned i=0; i<4; ++i) if (cell->cellName == temporaries[i]) {
      temporaries[i] = ""; //Mark as non-temporary
      //Set both modification flags
      modificationsDuringActivation=true;
      conf["edit"]["modified"]=true;
      break;
    }
  }

  libconfig::Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"]);
  for (unsigned i=0; i<4; ++i) if (temporaries[i].size()) {
    for (unsigned j=0; j<(unsigned)s.getLength(); ++j) {
      if (temporaries[i] == s[j].getName())
        s.remove(j--);
      else for (unsigned n=0; n<4; ++n)
        if (temporaries[i] == (const char*)s[j]["neighbours"][n])
          s[j]["neighbours"][n] = "";
    }
    for (unsigned c=0; c<ship->cells.size(); ++c)
      if (ship->cells[c]->cellName == temporaries[i]) {
        //Remove cell
        for (unsigned n=0; n<4; ++n) if (ship->cells[c]->neighbours[n])
          for (unsigned m=0; m<4; ++m)
            if (ship->cells[c]->neighbours[n]->neighbours[m] == ship->cells[c])
              ship->cells[c]->neighbours[n]->neighbours[m] = NULL;
        //Remove from paint list
        for (unsigned d=0; d<paintCells.size(); ++d)
          if (paintCells[d] == ship->cells[c]) {
            paintCells.erase(paintCells.begin() + d);
            break;
          }

        delete ship->cells[c];
        ship->cells.erase(ship->cells.begin() + (c--));

      }
  }

  manip->commitUndo();

  //manip->reloadShip();
  conf["edit"]["status_message"] = _(editor,cell_creation_brush_instructions);
}

bool CellCreationBrush::spawnTemporary(unsigned n) {
  const char* type = conf["edit"]["new_cell_type"];
  unsigned orientation = (strcmp("right", type)==0?
                          (int)conf["edit"]["rightt_orient"]
                         :0);

  Cell* active=NULL;

  //Find the active cell
  for (unsigned i=0; i<ship->cells.size() && !active; ++i)
    if (ship->cells[i]->cellName == activeCell)
      active = ship->cells[i];

  assert(active);

  if (n >= (unsigned)active->numNeighbours() || active->neighbours[n]) {
    return false;
  }

  //Try spawning a new cell there
  Cell* c;
  if      (strcmp("square", type)==0) c=new SquareCell(ship);
  else if (strcmp("circle", type)==0) c=new CircleCell(ship);
  else if (strcmp("right" , type)==0) c=new RightTCell(ship);
  else if (strcmp("equil" , type)==0) c=new EquTCell(ship);
  else {
    cerr << "FATAL: Unexpected new cell type: " << type << endl;
    exit(EXIT_SCRIPTING_BUG);
  }

  ship->cells.push_back(c);
  active->neighbours[n] = c;
  c->neighbours[orientation] = active;
  active->orientImpl();
  if (!detectInterconnections(c)) {
    //Can't insert here with this orientation
    //Erase cell from ship
    ship->cells.pop_back();
    for (unsigned i=0; i<4; ++i) if (c->neighbours[i])
      for (unsigned j=0; j<4; ++j)
        if (c->neighbours[i]->neighbours[j] == c) {
          c->neighbours[i]->neighbours[j] = NULL;
          break;
        }
    delete c;
    temporaries[n] = "";
    return false; //Can't spawn here
  }

  //OK, make entries in the config
  generateName(temporaries[n]);
  c->cellName = temporaries[n];

  libconfig::Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"]);
  //First, create new cell entry
  libconfig::Setting& cs(s.add(temporaries[n].c_str(), libconfig::Setting::TypeGroup));
  libconfig::Setting& ns(cs.add("neighbours", libconfig::Setting::TypeArray));
  cs.add("type", libconfig::Setting::TypeString) = type;
  for (unsigned m=0; m<4; ++m)
    ns.add(libconfig::Setting::TypeString) = (c->neighbours[m]? c->neighbours[m]->cellName.c_str() : "");
  //Now, create new bindings for the cell's neighbours
  for (unsigned m=0; m<4; ++m)
    if (c->neighbours[m])
      for (unsigned o=0; o<4; ++o)
        if (c == c->neighbours[m]->neighbours[o])
          s[c->neighbours[m]->cellName.c_str()]["neighbours"][o]=temporaries[n].c_str();

  //Set up painting
  paintCells.push_back(c);

  //OK, this spawning was successful
  return true;
}

void CellCreationBrush::generateName(string& name) {
  bool unique;
  unsigned syllables=2;
  libconfig::Setting& s(conf[(const char*)conf["edit"]["mountname"]]["cells"]);
  do {
    ++syllables;

    name="";

    //Add syllables
    for (unsigned sy=0; sy<syllables; ++sy) {
      name += consonants[rand() % lenof(consonants)];
      name += vowels[rand() % lenof(vowels)];
      if ((rand() & 0x7) == 0) name += "n";
    }

    //Check for uniqueness
    unique = !s.exists(name.c_str());
  } while (!unique);
}

bool CellCreationBrush::detectInterconnections(Cell* cell) {
  vector<Cell*>& cells(ship->cells);

  //We find neighbours by testing each against cell, and,
  //if the neighbour math would indicate they match, we bind them
  for(unsigned int i=0; i<cells.size(); ++i) {
    if (cells[i]==cell) continue;

    //cn=cell neighbour, on=other neighbour
    for (int cn=0; cn<cell    ->numNeighbours(); ++cn)
    for (int on=0; on<cells[i]->numNeighbours(); ++on) {
      if (cell->neighbours[cn]==cells[i]) goto nextCell;
      if (cells[i]->neighbours[on]==cell) goto nextCell;
      if (cell->neighbours[cn] || cells[i]->neighbours[on]) continue; //binding already exists

      //Check each edge point. If two of them are approx. equal, bind
      float cx=cell->getX(), cy=cell->getY(), ct=cell->getT()*pi/180.0f;
      float ox=cells[i]->getX(), oy=cells[i]->getY(), ot=cells[i]->getT()*pi/180.0f;
      float ca=ct + cell->edgeT(cn)*pi/180.0f;
      if (ca>2*pi) ca-=2*pi;
      float oa=ot + cells[i]->edgeT(on)*pi/180.0f;
      if (oa>2*pi) oa-=2*pi;
      float cd=cell->edgeD(cn);
      float od=cells[i]->edgeD(on);
      float cex=cx + cd*cos(ca);
      float oex=ox + od*cos(oa);
      float cey=cy + cd*sin(ca);
      float oey=oy + od*sin(oa);

      float dx=cex-oex, dy=cey-oey;
      float dist=sqrt(dx*dx + dy*dy);
      if (dist<STD_CELL_SZ/32) {
        cell->neighbours[cn]=cells[i];
        cells[i]->neighbours[on]=cell;
        goto nextCell;
      }
    }

    nextCell:;
  }

  //Check this cell against all others for collision
  //A cell cannot collide with one of its neighbours
  //or a neighbour's neighbour
  //We need to shrink the collision bounds on all
  //rects by 15% so that touching corners do not
  //trigger collisions
  for (unsigned i=0; i<cells.size(); ++i) {
    if (cells[i]==cell) continue;
    for (unsigned n=0; n<4; ++n) {
      if (cells[i] == cell->neighbours[n]) goto check_next_cell;
      if (cell->neighbours[n])
        for (unsigned m=0; m<4; ++m)
          if (cells[i] == cell->neighbours[n]->neighbours[m])
            goto check_next_cell;
    }

    {
      CollisionRectangle r1(*cell->getCollisionBounds()),
                         r2(*cells[i]->getCollisionBounds());
      float tx=0, ty=0;
      for (unsigned j=0; j<4; ++j) {
        tx += r1.vertices[j].first;
        ty += r1.vertices[j].second;
      }
      float cx = tx/4, cy=ty/4;
      for (unsigned j=0; j<4; ++j) {
        r1.vertices[j].first = cx + (r1.vertices[j].first - cx)*0.85f;
        r1.vertices[j].second= cy + (r1.vertices[j].second- cy)*0.85f;
      }
      tx=ty=0;
      for (unsigned j=0; j<4; ++j) {
        tx += r2.vertices[j].first;
        ty += r2.vertices[j].second;
      }
      cx = tx/4, cy=ty/4;
      for (unsigned j=0; j<4; ++j) {
        r2.vertices[j].first = cx + (r2.vertices[j].first - cx)*0.85f;
        r2.vertices[j].second= cy + (r2.vertices[j].second- cy)*0.85f;
      }

      float dx = r1.vertices[0].first - r2.vertices[0].first;
      float dy = r1.vertices[0].second - r2.vertices[0].second;
      float maxDist = STD_CELL_SZ*3;
      if (dx*dx + dy*dy < maxDist
      &&  rectanglesCollide(*cell->getCollisionBounds(), *cells[i]->getCollisionBounds())) {
        if (whyNotCells.end() == find(whyNotCells.begin(), whyNotCells.end(), cells[i]))
          whyNotCells.push_back(cells[i]);
        return false; //Collision
      }
    }

    check_next_cell:;
  }

  return true;
}
