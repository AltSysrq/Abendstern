/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.04
 * @brief Implementation of NetworkConnection class
 */

#include <iostream>

#include <SDL.h>

#include "network_connection.hxx"
#include "network_assembly.hxx"
#include "network_geraet.hxx"
#include "antenna.hxx"
#include "io.hxx"

#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"

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
  for (geraete_t::const_iterator it = geraete.begin();
       it != geraete.end(); ++it)
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

      //Accept packet; does the channel exist?
      chanmap_t::const_iterator it = locchan.find(chan);
      if (it != locchan.end()) {
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
