/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.04
 * @brief Implementation of the NetworkAssembly class
 */

#include <algorithm>
#include <vector>

#include "network_assembly.hxx"
#include "network_connection.hxx"
#include "network_geraet.hxx"
#include "tuner.hxx"
#include "antenna.hxx"
#include "abuhops.hxx"

using namespace std;;

NetworkAssembly::NetworkAssembly(GameField* field_, Antenna* antenna_)
: tuner(new Tuner), wfo(antenna_), field(field_), antenna(antenna_)
{
  antenna->tuner = tuner;
  field->networkAssembly = this;

  abuhops::ensureRegistered();
}

NetworkAssembly::~NetworkAssembly() {
  for (unsigned i=0; i<connections.size(); ++i)
    delete connections[i];
  for (unsigned i=0; i<packetProcessors.size(); ++i)
    delete packetProcessors[i];
  delete tuner;
  antenna->tuner = antenna->defaultTuner;

  abuhops::ensureRegistered();
  //Since this class is the one controlling abuhops, we must disconnect it when
  //destroyed, since it will no longer receive updates.
  abuhops::stopList();
  abuhops::bye();
}

void NetworkAssembly::removeConnection(unsigned ix) noth {
  delete connections[ix];
  connections.erase(connections.begin()+ix);
}

void NetworkAssembly::removeConnection(NetworkConnection* cxn) noth {
  connections.erase(find(connections.begin(), connections.end(), cxn));
  delete cxn;
}

void NetworkAssembly::addPacketProcessor(PacketProcessor* proc) noth {
  packetProcessors.push_back(proc);
}

void NetworkAssembly::addConnection(NetworkConnection* cxn) noth {
  connections.push_back(cxn);

  //Notify connection of all known objects
  for (set<GameObject*>::const_iterator it = knownObjects.begin();
       it != knownObjects.end(); ++it)
    cxn->objectAdded(*it);
}

void NetworkAssembly::update(unsigned et) noth {
  abuhops::update(et);
  wfo.update(et);
  antenna->processIncomming();
  for (unsigned i=0; i<connections.size(); ++i)
    connections[i]->update(et);
}

void NetworkAssembly::setFieldSize(float w, float h) throw() {
  for (unsigned i=0; i<connections.size(); ++i)
    connections[i]->setFieldSize(w,h);
}

void NetworkAssembly::changeField(GameField* f) throw() {
  field = f;
  setFieldSize(f->width, f->height);
  f->networkAssembly = this;
}

void NetworkAssembly::objectAdded(GameObject* go) throw() {
  if (!go->isExportable || go->isRemote)
    return;

  knownObjects.insert(go);
  for (unsigned i=0; i<connections.size(); ++i)
    connections[i]->objectAdded(go);
}

void NetworkAssembly::objectRemoved(GameObject* go) throw() {
  knownObjects.erase(go);
  for (unsigned i=0; i<connections.size(); ++i)
    connections[i]->objectRemoved(go);
}
