/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkAssembly class
 */
#ifndef NETWORK_ASSEMBLY_HXX_
#define NETWORK_ASSEMBLY_HXX_

#include <vector>
#include <set>

#include "src/core/aobject.hxx"

class Antenna;
class Tuner;
class NetworkConnection;
class PacketProcessor;
class GameField;
class GameObject;

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
  Tuner* tuner;

  /* Contains all local, exportable GameObjects known to the assembly. */
  std::set<GameObject*> knownObjects;

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
  Tuner* getTuner() const noth { return tuner; }
  /**
   * Returns the number of NetworkConnections contained in this
   * NetworkAssembly.
   */
  unsigned numConnections() const noth { return connections.size(); }
  /**
   * Returns the NetworkConnection at the given index.
   */
  NetworkConnection* getConnection(unsigned ix) const noth {
    return connections[ix];
  }
  /**
   * Adds the given NetworkConnection to this assembly.
   */
  void addConnection(NetworkConnection*) noth;
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

  /**
   * Resizes the internal fields of all NetworkConnections to the given
   * dimensions.
   * @see NetworkConnection::setFieldSize()
   * @see GameField::width
   * @see GameField::height
   */
  void setFieldSize(float,float) throw();

  /**
   * Called whenever a new GameObject is inserted into the field.
   */
  void objectAdded(GameObject*) throw();
  /**
   * Called whenever a GameObject is removed from the field.
   */
  void objectRemoved(GameObject*) throw();
};

#endif /* NETWORK_ASSEMBLY_HXX_ */
