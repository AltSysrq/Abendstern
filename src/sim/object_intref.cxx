/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/object_intref.hxx
 */

/*
 * object_intref.cxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#include <map>
#include <iostream>
#include "object_intref.hxx"

using namespace std;

//This is left uninitialized, because we never care about any
//meaning behind its value, only what its value is
//In DEBUG mode, do initialize it so Valgrind doesn't complain
#ifndef DEBUG
static int nextInt;
#else
static int nextInt=0;
#endif

static map<int,const ObjectIntref*> references;

static int getNewReference(const ObjectIntref* oi) {
  ++nextInt;
  references[nextInt] = oi;
  return nextInt;
}

ObjectIntref::ObjectIntref(GameObject*const& r)
: pointer(r), reference(getNewReference(this))
{ }

ObjectIntref::~ObjectIntref() {
  references.erase(references.find(reference));
}

GameObject* ObjectIntref::get(int r) {
  map<int,const ObjectIntref*>::const_iterator it=references.find(r);
  if (it == references.end()) return NULL;
  else return (*it).second->pointer;
}
