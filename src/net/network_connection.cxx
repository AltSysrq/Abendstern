/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.04
 * @brief Implementation of NetworkConnection class
 */

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include <SDL.h>

#include "network_connection.hxx"
#include "network_assembly.hxx"
#include "network_geraet.hxx"
#include "antenna.hxx"
#include "tuner.hxx"
#include "io.hxx"
#include "synchronous_control_geraet.hxx"
#include "async_ack_geraet.hxx"
#include "lat_disc_geraet.hxx"

#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/core/lxn.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

/* How long to wait without hearing any response before
 * killing the connection.
 */
#define DISCONNECT_TIME 2048

/* Maximum distance between reference and transient where we
 * still export that transient.
 */
#define TRANSIENT_DIST 7.0f

NetworkConnection::geraetNumMap_t* NetworkConnection::geraetNumMap;

NetworkConnection::NetworkConnection(NetworkAssembly* assembly_,
                                     const Antenna::endpoint& endpoint_,
                                     bool incomming)
: field(assembly_->field->width, assembly_->field->height),
  nextOutSeq(incomming? 0 : 1),
  greatestSeq(0),
  latency(128),
  lastIncommingTime(SDL_GetTicks()),
  status(incomming? Established : Connecting),
  timeSinceRetriedTransients(0),
  endpoint(endpoint_),
  parent(assembly_),
  scg(new SynchronousControlGeraet(this, incomming)),
  aag(new AsyncAckGeraet(this)),
  ldg(new LatDiscGeraet(this))
{
  inchannels[0] = scg;
  outchannels[0] = scg;

  if (incomming)
    scg->transmitAck(0);

  parent->getTuner()->connect(endpoint, this);
}

NetworkConnection::~NetworkConnection() {
  parent->getTuner()->disconnect(endpoint);

  for (inchannels_t::const_iterator it = inchannels.begin();
       it != inchannels.end(); ++it)
    if (it->second->deletionStrategy != InputNetworkGeraet::DSIntrinsic)
      delete it->second;
  for (outchannels_t::const_iterator it = outchannels.begin();
       it != outchannels.end(); ++it)
    if (it->second->deletionStrategy == OutputNetworkGeraet::DSNormal)
      delete it->second;

  delete aag;
  delete scg;
}

void NetworkConnection::update(unsigned et) noth {
  field.update(et);
  for (inchannels_t::const_iterator it = inchannels.begin();
       it != inchannels.end(); ++it)
    it->second->inputUpdate(et);
  for (outchannels_t::const_iterator it = outchannels.begin();
       it != outchannels.end(); ++it)
    it->second->update(et);

  //TODO: Only process objects if in Ready status
  timeSinceRetriedTransients += et;
  if (timeSinceRetriedTransients > 128) {
    candidateExports.insert(candidateExports.end(),
                            ignoredExports.begin(),
                            ignoredExports.end());
    ignoredExports.clear();
    timeSinceRetriedTransients = 0;
  }
  //Export candidates that are close enough or non-transient
  while (!candidateExports.empty()) {
    GameObject* go = candidateExports.front();
    candidateExports.pop_front();
    if (!go->isTransient || distanceOf(go) < TRANSIENT_DIST*TRANSIENT_DIST) {
      actualExports.insert(go);
      //TODO: export it
    } else {
      ignoredExports.insert(go);
    }
  }

  if (lastIncommingTime + DISCONNECT_TIME < SDL_GetTicks()) {
    scg->closeConnection("Timed out", "timed_out");
  }
}

void NetworkConnection::process(const Antenna::endpoint& source,
                                Antenna* antenna, Tuner* tuner,
                                const byte* data, unsigned datlen)
noth {
  if (datlen < 4) {
    //Packet does not even have a header
    #ifdef DEBUG
    cerr << "Warning: Dropping packet of length " << datlen
         << " from source " << source << endl;
    #endif
    return;
  }

  channel chan;
  seq_t seq;
  io::read(data, seq);
  io::read(data, chan);
  datlen -= 4;
  cout << "Receive seq=" << seq << " on chan=" << chan
       << " with length=" << datlen << " from " << source << endl;

  //Range check
  if (seq-greatestSeq < 1024 || greatestSeq-seq < 1024) {
    //Dupe check
    if (recentlyReceived.count(seq) == 0) {
      //Possibly update greatestSeq
      if (seq-greatestSeq < 1024)
        greatestSeq = seq;

      //OK, add to set and queue, possibly trim both
      recentlyReceived.insert(seq);
      recentlyReceivedQueue.push_back(seq);
      if (recentlyReceivedQueue.size() > 1024) {
        recentlyReceived.erase(recentlyReceivedQueue.front());
        recentlyReceivedQueue.pop_front();
      }

      //Update time of most recent receive
      lastIncommingTime = SDL_GetTicks();

      //Accept packet; does the channel exist?
      inchannels_t::const_iterator it = inchannels.find(chan);
      if (it != inchannels.end()) {
        it->second->receive(seq, data, datlen);
      } else {
        #ifdef DEBUG
        cerr << "Warning: Dropping packet to closed channel " << chan
             << " from source " << source << endl;
        #endif
      }
    }
  } else {
    #ifdef DEBUG
    cerr << "Warning: Dropping packet of seq " << seq
         << " due to range check; greatestSeq = " << greatestSeq
         << "; from source " << source << endl;
    #endif
  }
}

NetworkConnection::seq_t NetworkConnection::seq() noth {
  //Check for network collapse
  if (locked[nextOutSeq]) {
    //Kill the connection
    //First, unlock everything so infinite recursion does not result
    locked.reset();
    scg->closeConnection("Network collapse", "network_collapse");
  }
  return nextOutSeq++;
}

NetworkConnection::seq_t NetworkConnection::writeHeader(byte*& dst,
                                                        channel chan)
noth {
  seq_t s = seq();
  io::write(dst, s);
  io::write(dst, chan);
  return s;
}

void NetworkConnection::send(const byte* data, unsigned len) throw() {
  try {
    parent->antenna->send(endpoint, data, len);
  } catch (asio::system_error e) {
    cerr << "Network system error: " << e.what() << endl;
    status = Zombie;
    SL10N(disconnectReason, network, system_error);
  }
}

NetworkConnection::geraet_num
NetworkConnection::registerGeraetCreator(geraet_creator fun,
                                         geraet_num preferred) {
  static bool hasInitialised = false;
  static geraet_num nextAuto = 32768;
  //Allocate map if not done yet
  if (!hasInitialised) {
    geraetNumMap = new geraetNumMap_t;
    hasInitialised = true;
  }

  geraet_num number = preferred;

  if (number == (geraet_num)~(geraet_num)0)
    number = nextAuto++;

  if (geraetNumMap->count(number)) {
    cerr << "FATAL: Duplicate Geraet number: " << number << endl;
    exit(EXIT_PROGRAM_BUG);
  }

  geraetNumMap->insert(make_pair(number, fun));
  return number;
}

NetworkConnection::geraet_creator
NetworkConnection::getGeraetCreator(geraet_num num) {
  assert(geraetNumMap->count(num));
  return (*geraetNumMap)[num];
}

void NetworkConnection::setReference(GameObject* go) throw() {
  remoteReferences.push_back(go);
}

void NetworkConnection::unsetReference(GameObject* go) throw() {
  vector<GameObject*>::iterator it =
      find(remoteReferences.begin(), remoteReferences.end(), go);
  if (it != remoteReferences.end())
    remoteReferences.erase(it);
}

float NetworkConnection::distanceOf(const GameObject* go) const throw() {
  float mindist = INFINITY;
  for (unsigned i=0; i<remoteReferences.size(); ++i) {
    const GameObject* that = remoteReferences[i];
    float dx = go->getX() - that->getX();
    float dy = go->getY() - that->getY();
    float d = dx*dx + dy*dy;
    if (d < mindist)
      mindist = d;
  }
  return mindist;
}

void NetworkConnection::objectAdded(GameObject* go) throw() {
  candidateExports.push_back(go);
}

void NetworkConnection::objectRemoved(GameObject* go) throw() {
  deque<GameObject*>::iterator dit =
      find(candidateExports.begin(), candidateExports.end(), go);
  if (dit != candidateExports.end())
    candidateExports.erase(dit);

  actualExports.erase(go);
  ignoredExports.erase(go);
  unsetReference(go);
}
