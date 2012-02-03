/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkAssembly class
 */
#ifndef NETWORK_ASSEMBLY_HXX_
#define NETWORK_ASSEMBLY_HXX_

#include <vector>

#include "src/core/aobject.hxx"

class Antenna;
class Tuner;
class NetworkConnection;
class PacketProcessor;

/**
 * Manages the various objects that are used to provide a functional network
 * system.
 *
 * This is essentially a convenience class for managing the list of connections
 * and the memory used by the various PacketProcessors.
 */
class NetworkAssembly: public AObject {
  std::vector<NetworkConnection*> connections;
  std::vector<PacketProcessor*> packetProcessors;
  Tuner tuner;

  ///Not implemented, do not use
  NetworkAssembly(const NetworkAssembly&);

public:
  /**
   * The GameField used by the networking system.
   */
  GameField*const field;
  /**
   * The Antenna used for communication.
   */
  Antenna*const antenna;

  /**
   * Constructs a NetworkAssembly on the given GameField and Antenna.
   * There are initially no connections or packet processors.
   *
   * @param field the GameField the network operates on
   * @param antenna the Antenna to use for sending and receiving of messages
   */
  NetworkAssembly(GameField* field, Antenna* antenna);
  virtual ~NetworkAssembly();

  /**
   * Returns the Tuner associated with this NetworkAssembly.
   */
  Tuner* getTuner() noth;
  /**
   * Returns the number of NetworkConnections contained in this
   * NetworkAssembly.
   */
  unsigned numConnections() const noth;
  /**
   * Returns the NetworkConnection at the given index.
   */
  NetworkConnection* getConnection(unsigned ix) const noth;
  /**
   * Removes and deletes the NetworkConnection at the given index.
   */
  void removeConnection(unsigned ix) noth;
  /**
   * Removes and deletes the given NetworkConnection, which must
   * exist within this NetworkAssembly's connection list.
   */
  void removeConnection(NetworkConnection*) noth;
  /**
   * Adds the given PacketProcessor to those managed by this instance.
   * The PacketProcessor will be deleted when this NetworkAssembly is deleted.
   */
  void addPacketProcessor(PacketProcessor*) noth;
  /**
   * Updates all NetworkConnections.
   * @param et the number of milliseconds that have elapsed since the last
   * call to update()
   */
  void update(unsigned et) noth;
};

#endif /* NETWORK_ASSEMBLY_HXX_ */
