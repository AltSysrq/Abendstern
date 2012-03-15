#ifndef NETWORK_GAME_HXX_
#define NETWORK_GAME_HXX_

/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.15
 * @brief Contains classes to bridge the C++ networking to the Tcl game classes.
 */

#include <string>
#include <map>

#include "src/core/aobject.hxx"
#include "globalid.hxx"
#include "connection_listener.hxx"
#include "network_assembly.hxx"

class NetworkConnection;

/**
 * Contains the data associated with a single Peer, which is shared across
 * possibly multiple (non-coincident) NetworkConnection objects (eg, across
 * temporary drops).
 */
class Peer: public AObject {
public:
  ///The global ID information for the peer, for purposes of connections
  GlobalID gid;
  ///The numeric ID of the peer, for purposes of determing who's overseer
  unsigned nid;
  ///Whether this peer is ready to perform the role of overseer
  bool overseerReady;
  ///The current NetworkConnection associated with this peer
  NetworkConnection* cxn;
};

/**
 * Abstract class used to relay events to the Tcl game system.
 */
class NetIface: public AObject {
public:
  ///Called when the given Peer has been created
  virtual void addPeer(Peer*) = 0;
  ///Called immediately before the given Peer will be removed
  virtual void delPeer(Peer*) = 0;
  ///Called when the overseer Peer changes
  virtual void setOverseer(Peer*) = 0;

  ///Called when a broadcast message from the given peer is received
  virtual void receiveBroadcast(Peer*, const char*) = 0;
  ///Called when a to-overseer message from the given peer is received
  virtual void receiveOverseer(Peer*, const char*) = 0;
  ///Called when a unicast message from the given peer is received
  virtual void receiveUnicast(Peer*, const char*) = 0;

  ///Called when all network connectivity has been lost, with the given reason.
  virtual void connectionLost(const char*) = 0;

  ///Formats a Peer* into the string format to use for, eg, the O message.
  virtual const char* formatPeer(Peer*) = 0;
  ///Extracts the LAN IP address from the given peer string
  virtual const char* getLanIp(const char*) = 0;
  ///Extracts the Internet IP address from the given peer string
  virtual const char* getInetIp(const char*) = 0;
  ///Extracts the LAN port nmber from the given peer string
  virtual unsigned getLanPort(const char*) = 0;
  ///Extracts the Internet port number from the given peer string
  virtual unsigned getInetPort(const char*) = 0;
  /**
   * Given the peer strings for the local host and a remote peer, return
   * true if the LAN address/port pair should be used, false if the Internet
   * pair should be used for establishing a connection.
   */
  virtual bool useLanAddress(const char* local, const char* remote) = 0;
};

class NetworkGame: public ConnectionListener {
  std::string lastDisconnectReason;
  typedef std::map<NetworkConnection*, Peer> peers_t;
  peers_t peers;

  NetworkAssembly assembly;
  NetIface*const interface;

  unsigned localNid;
};

#endif /* NETWORK_GAME_HXX_ */
