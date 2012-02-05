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

#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

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
  scg(NULL) //TODO: replace with actual object later
{
  //TODO: put SCG in channel map
}

NetworkConnection::~NetworkConnection() {
  //scg is stored within the channel map, so we don't have to explicitly
  //delete it.
  for (inchannels_t::const_iterator it = inchannels.begin();
       it != inchannels.end(); ++it)
    delete it->second;
  for (outchannels_t::const_iterator it = outchannels.begin();
       it != outchannels.end(); ++it)
    delete it->second;
}

void NetworkConnection::update(unsigned et) noth {
  //TODO: do something
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
