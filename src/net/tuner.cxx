/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.31
 * @brief Implementation of Tuner network class
 */

#include <asio.hpp>
#include <map>
#include <vector>
#include <utility>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <algorithm>

#include "antenna.hxx"
#include "tuner.hxx"
#include "packet_processor.hxx"

using namespace std;

Tuner::Tuner() {}

void Tuner::connect(const Antenna::endpoint& source,
                    PacketProcessor* processor)
noth {
  connections[source] = processor;
}

PacketProcessor* Tuner::disconnect(const Antenna::endpoint& source) noth {
  connections_t::iterator it = connections.find(source);
  assert(it != connections.end());
  PacketProcessor* pp = it->second;
  connections.erase(it);
  return pp;
}

void Tuner::trigger(const byte* data, unsigned len, PacketProcessor* processor)
noth {
  //Copy into a vector
  vector<byte> vdat(data, data+len);
  //Remove any matching one that exists
  for (headers_t::iterator it = headers.begin(); it != headers.end(); ++it) {
    if (it->first == vdat) {
      headers.erase(it);
      break;
    }
  }

  //Insert before the first trigger that is shorter than or the same
  //length as it
  headers_t::iterator it = headers.begin();
  while (it != headers.end() && it->first.size() > len)
    ++it;
  //Found the position, now insert it
  headers.insert(it, make_pair(vdat,processor));
}

PacketProcessor* Tuner::untrigger(const byte* data, unsigned len) noth {
  vector<byte> vdat(data, data+len);

  //Search for it
  headers_t::iterator it = headers.begin();
  while (it->first != vdat) ++it;

  //Extract current processor
  PacketProcessor* pp = it->second;

  //Remove and we're done
  headers.erase(it);
  return pp;
}

void Tuner::receivePacket(const Antenna::endpoint& source,
                          Antenna* antenna,
                          const byte* data, unsigned datlen)
noth {
  //First, check for a connection to this source
  connections_t::const_iterator it = connections.find(source);
  if (it != connections.end()) {
    //Connected peer
    it->second->process(source, antenna, this, data, datlen);
  } else {
    //Unknown peer, check for header recognition
    for (headers_t::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
      if (datlen >= it->first.size()
      &&  equal(data, data+it->first.size(),
                it->first.begin())) {
        //Header matches
        it->second->process(source, antenna, this, data, datlen);
        break;
      }
    }
    //Drop any unrecognised packets silently
  }
}
