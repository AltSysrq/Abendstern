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

using namespace std;;

NetworkAssembly::NetworkAssembly(GameField* field_, Antenna* antenna_)
: tuner(new Tuner), field(field_), antenna(antenna_)
{
}

NetworkAssembly::~NetworkAssembly() {
  for (unsigned i=0; i<connections.size(); ++i)
    delete connections[i];
  for (unsigned i=0; i<packetProcessors.size(); ++i)
    delete packetProcessors[i];
  delete tuner;
}

void NetworkAssembly::removeConnection(unsigned ix) noth {
  connections.erase(connections.begin()+ix);
}

void NetworkAssembly::removeConnection(NetworkConnection* cxn) noth {
  connections.erase(find(connections.begin(), connections.end(), cxn));
}

void NetworkAssembly::addPacketProcessor(PacketProcessor* proc) noth {
  packetProcessors.push_back(proc);
}

void NetworkAssembly::addConnection(NetworkConnection* cxn) noth {
  connections.push_back(cxn);
}

void NetworkAssembly::update(unsigned et) {
  for (unsigned i=0; i<connections.size(); ++i)
    connections[i]->update(et);
}