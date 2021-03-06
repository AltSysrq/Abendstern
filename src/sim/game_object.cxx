/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/game_object.hxx
 */

#include <iostream>
#include <cstdlib>
#include <typeinfo>

#include "game_object.hxx"
#include "game_field.hxx"
#include "src/globals.hxx"
#include "collision.hxx"
#include "objdl.hxx"
#include "src/exit_conditions.hxx"
#include "src/net/network_assembly.hxx"

using namespace std;

const vector<CollisionRectangle*>* GameObject::getCollisionBounds() noth {
  return &collisionBounds;
}

void GameObject::del() noth {
  ci.isDead = true;
  deleteCommon();
  field->deleteNextFrame.push_back(this);
  field->removeFromInsertQueue(this);
  if (field->networkAssembly)
    field->networkAssembly->objectRemoved(this);
}

void GameObject::deleteCommon() noth {
  for (ObjDL* odl=listeners; odl; odl=odl->nxt)
    odl->ref=NULL;
  listeners=NULL;
}

GameObject::~GameObject() {
  deleteCommon();
}
