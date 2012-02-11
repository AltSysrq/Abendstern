/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.04
 * @brief Implementation of NetworkConnection class
 */

#include <iostream>
#include <cstdlib>
#include <cassert>

#include <SDL.h>

#include "network_connection.hxx"
#include "network_assembly.hxx"
#include "network_geraet.hxx"
#include "antenna.hxx"
#include "io.hxx"
#include "synchronous_control_geraet.hxx"

#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/core/lxn.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

/* How long to wait without hearing any response before
 * killing the connection.
 */
#define DISCONNECT_TIME 2048

NetworkConnection::geraetNumMap_t* NetworkConnection::geraetNumMap;

NetworkConnection::NetworkConnection(NetworkAssembly* assembly_,
                                     const Antenna::endpoint& endpoint_,
                                     bool incomming)
: field(assembly_->field->width, assembly_->field->height),
  nextOutSeq(0),
  greatestSeq(0),
  latency(0),
  lastIncommingTime(SDL_GetTicks()),
  status(incomming? Established : Connecting),
  endpoint(endpoint_),
  parent(assembly_),
  scg(new SynchronousControlGeraet(this, incomming))
{
  inchannels[0] = scg;
  outchannels[0] = scg;

  if (incomming)
    scg->transmitAck(0);

  parent->getTuner()->connect(endpoint, this);
}

NetworkConnection::~NetworkConnection() {
  parent->getTuner()->disconnect(endpoint);

  //Other Geräte depend on the SCG in deletion, so delete it
  //at the end and ignore it within the loops
  for (inchannels_t::const_iterator it = inchannels.begin();
       it != inchannels.end(); ++it)
    if (it->second != scg) delete it->second;
  for (outchannels_t::const_iterator it = outchannels.begin();
       it != outchannels.end(); ++it)
    if (it->second != scg) delete it->second;
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

InputNetworkGeraet* NetworkConnection::getGeraetByNum(geraet_num num) noth {
  return geraete[num];
}

NetworkConnection::seq_t NetworkConnection::seq() noth {
  return nextOutSeq++;
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

  if (number == ~(geraet_num)0)
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
