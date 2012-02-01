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

#include "antenna.hxx"
#include "tuner.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

Tuner::Tuner() {}

void Tuner::connect(const Antenna::endpoint& source,
                    PacketProcessor* processor)
noth {
  connections[source] = processor;
}

PacketProcessor* Tuner::disconnect(const Antenna::endpoint& source) noth {
  map<Antenna::endpoint, PacketProcessor*>::iterator it =
      connections.find(source);
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
  for (list<pair<vector<byte>,PacketProcessor*> >::iterator it =
       headers.begin(); it != headers.end(); ++it) {
    if (it->first == vdat) {
      headers.erase(it);
      break;
    }
  }

  //Insert before the first trigger that is shorter than or the same
  //length as it
  list<pair<vector<byte>,PacketProcessor*> >::iterator it = headers.begin();
  while (it != headers.end() && it->first.size() > len)
    ++it;
  //Found the position, now insert it
  headers.insert(it, make_pair(vdat,processor));
}

PacketProcessor* Tuner::untrigger(const byte* data, unsigned len) noth {
  vector<byte> vdat(data, data+len);

  //Search for it
  list<pair<vector<byte>,PacketProcessor*> >::iterator it = headers.begin();
  while (it->first != vdat) ++it;

  //Extract current processor
  PacketProcessor* pp = it->second;

  //Remove and we're done
  headers.erase(it);
  return pp;
}
