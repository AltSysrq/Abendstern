/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/spectator.hxx.
 */

/*
 * spectator.cxx
 *
 *  Created on: 08.10.2011
 *      Author: jason
 */

#include <vector>
#include <typeinfo>
#include <cstdlib>

#include "spectator.hxx"
#include "src/ship/ship.hxx"
#include "src/sim/game_field.hxx"

#define MAX_WITHOUT_REF_TIME 5000

Spectator::Spectator(Ship* s, bool insta)
: GameObject(s->getField(), s->getX(), s->getY(), s->getVX(), s->getVY()),
  ref(insta? NULL : s),
  timeWithoutReference(0),
  insignia(0),
  isInsigniaRequired(false),
  theta(s->getRotation()),
  isAlive(true)
{
  includeInCollisionDetection = false;
  decorative = true;
  okToDecorate();
}

Spectator::Spectator(GameField* f)
: GameObject(f, 0, 0, 0, 0),
  ref(NULL),
  timeWithoutReference(0),
  insignia(0),
  isInsigniaRequired(false),
  theta(0),
  isAlive(true)
{
  includeInCollisionDetection = false;
  decorative = true;
  okToDecorate();
}

bool Spectator::update(float et) noth {
  if (ref.ref && !((Ship*)ref.ref)->hasPower())
    ref.assign(NULL);

  if (!ref.ref) {
    timeWithoutReference += et;
    if (timeWithoutReference > MAX_WITHOUT_REF_TIME) {
      nextReference();
      timeWithoutReference = 0;
    }
  }

  //ref may be changed by the above
  if (ref.ref) {
    x = ref.ref->getX();
    y = ref.ref->getY();
    vx = ref.ref->getVX();
    vy = ref.ref->getVY();
    theta = ref.ref->getRotation();
  } else {
    x += vx * et;
    y += vy * et;
  }

  if (x < 0)             x = 0;
  if (y < 0)             y = 0;
  if (x >= field->width) x = field->width-0.001f;
  if (y >= field->height)y = field->height-0.001f;

  return isAlive;
}

void Spectator::draw() noth {}
float Spectator::getRotation() const noth { return theta; }
float Spectator::getRadius() const noth { return 0.001f; }

void Spectator::nextReference() {
  vector<Ship*> possibilities;

  bool hasFriend = false;
  //Scan the field for applicable Ships.
  //If the first try with the insignia check fails, try again.
  for (bool isInsigniaRequired = this->isInsigniaRequired, firstRun = true;
       firstRun || (!isInsigniaRequired && possibilities.empty() && !hasFriend);
       firstRun = false, isInsigniaRequired ^= true) {
    //Actual scan
    for (unsigned i=0; i<field->size(); ++i) {
      GameObject* go = field->at(i);
      if (go->getClassification() == ClassShip) {
        Ship* s = (Ship*)go;
        if (s == ref.ref) {
          //Check for hasFriend
          //This is needed so that the loop works correctly if exactly one friendly
          //ship exists.
          if (insignia == s->insignia) hasFriend=true;
          continue; //We want to /change/
        }
        if (!s->hasPower()) continue; //dead
        if (isInsigniaRequired && insignia != s->insignia) continue; //Not on the right team

        //OK
        possibilities.push_back(s);
      }
    }
  }

  if (possibilities.empty()) return;


  //Select one and we're done
  ref.assign(possibilities[rand()%possibilities.size()]);
}

void Spectator::requireInsignia(unsigned long i) {
  isInsigniaRequired = true;
  insignia = i;
}

void Spectator::kill() {
  isAlive = false;
}

Ship* Spectator::getReference() const {
  return (Ship*)ref.ref;
}
