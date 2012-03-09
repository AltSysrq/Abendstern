/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Implementation of GameObject Geräte base classes.
 */

#include <algorithm>

#include "object_geraet.hxx"
#include "network_assembly.hxx"
#include "synchronous_control_geraet.hxx"

#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"

using namespace std;

#define UPDATE_DELAY 128

ImportedGameObject::ImportedGameObject(unsigned sz, NetworkConnection* cxn)
: InputBlockGeraet(sz, cxn->aag),
  field(cxn->parent->field),
  object(NULL),
  created(false)
{
}

ImportedGameObject::~ImportedGameObject() {
  if (object) destroy(false);
}

void ImportedGameObject::modified() throw() {
  if (!created) {
    construct();
    created = true;
    fill(dirty.begin(), dirty.end(), false);
    if (object)
      field->add(object);
  } else if (object) {
    update();
    fill(dirty.begin(), dirty.end(), false);
  }
}

void ImportedGameObject::destroy(bool) throw() {
  object->collideWith(object);
  field->remove(object);
  object->del();
  object = NULL;
}

ExportedGameObject::ExportedGameObject(unsigned sz, NetworkConnection* cxn,
                                       GameObject* local_, GameObject* remote_)
: OutputBlockGeraet(sz, cxn->aag),
  timeUntilNextUpdate(UPDATE_DELAY),
  alive(true),
  local(local_),
  remote(remote_)
{
  cxn->field.add(remote);
}

ExportedGameObject::~ExportedGameObject() {
  remote->collideWith(remote);
  cxn->field.remove(remote);
  remote->del();
}

void ExportedGameObject::update(unsigned et) throw() {
  timeUntilNextUpdate -= et;
  if (local.ref) {
    //Local object still is alive, see if we should update it.
    if (timeUntilNextUpdate <= 0 && shouldUpdate()) {
      updateRemote();
      dirty = true;
      timeUntilNextUpdate = UPDATE_DELAY;
    }
  } else {
    //If we believed the object was still alive, send death update.
    if (alive) {
      alive = false;
      destroyRemote();
      dirty = true;
    } else {
      //Close channel once all is sent.
      if (isSynchronised()) {
        cxn->closeChannelWhenSafe(channel);
        return;
      }
    }
  }

  OutputBlockGeraet::update(et);
}

void ExportedGameObject::forceUpdate() throw() {
  updateRemote();
  dirty = true;
}
