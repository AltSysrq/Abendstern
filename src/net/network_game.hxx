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
#include "network_assembly.hxx"

class NetworkConnection;
class GameAdvertiser;
class GameDiscoverer;
class GameField;

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
  ///The cumulative number of times a connection attempt was made. This is
  ///decremented if non-zero once every so often. Reconnect attempts will not be
  ///made if this exceeds a certain value.
  unsigned connectionAttempts;
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

  ///Called when a datp alteration is received from the given Peer.
  ///Return true if the changes are accepted.
  virtual bool alterDatp(Peer*, const char* key, const char* val) = 0;
  ///Called when a dats alteration is received from the current overseer.
  ///Return true if the changes arv accepted.
  virtual bool alterDats(const char* key, const char* val) = 0;

  ///Called when all network connectivity has been lost, with the given reason.
  virtual void connectionLost(const char*) = 0;
};

/**
 * Provides a façade to the networking system which is easily accessible by Tcl.
 * While it primarily serves to manage associating Peers with
 * NetworkConnections (including peer discovery and topology discovery), it also
 * has some secondary utilities, such as game discovery, which would be hard
 * for Tcl to use by itself (since it requires manipulation of asio classes).
 */
class NetworkGame: public AObject {
  //The reason given (localised) for the most recent disconnect
  std::string lastDisconnectReason;
  //Map of NetworkConnection*s to Peer*s.
  typedef std::map<NetworkConnection*, Peer*> peers_t;
  peers_t peers;
  //The current overseer
  Peer* overseer;

  //Peer representing the local system
  Peer localPeer;

  NetworkAssembly assembly;
  //The current NetIface to use, or NULL if none
  NetIface* interface;

  GameAdvertiser* advertiser;
  GameDiscoverer* discoverer;

public:
  /**
   * Constructs a NetworkGame with the given initial GameField.
   */
  NetworkGame(GameField*);

  /**
   * Returns a pointer to the Peer object representing the local peer.
   */
  Peer* getLocalPeer() throw() { return &localPeer; }
  /**
   * Returns the Peer* which is the current overseer, or NULL for the local
   * peer.
   */
  Peer* getOverseer() const throw() { return overseer; }
  /**
   * Returns the stated reason for the most recent connection failure.
   */
  const std::string& getDisconnectReason() const throw() {
    return lastDisconnectReason;
  }

  /**
   * Alters the NetIface used. May be NULL.
   */
  void setNetIface(NetIface*) throw();

  /**
   * Initiates game advertising if not done already, then sets the gameMode
   * parm appropriately.
   */
  void setAdvertising(const char* gameMode) throw();
  /**
   * Terminates any ongoing game advertisement.
   */
  void stopAdvertising() throw();

  /**
   * Begins a scan using a GameDiscoverer. If a scan is currently ongoing,
   * nothing happens.
   */
  void startDiscoveryScan() throw();
  /**
   * Returns the current progress of the GameDiscoverer scan, between 0 and 1
   * inclusive. Returns -1 if no scan is in progress.
   */
  float discoveryScanProgress() const throw();
  /**
   * Returns whether the current scan has completed.
   * Returns false if no scan has been initiated.
   */
  bool discoveryScanDone() const throw();
  /**
   * Returns a Tcl list of scan results, suitable for display to the user.
   * To start a game.
   */
  std::string getDiscoveryResults() throw();

  /**
   * Begins listining for new connections. This is used to start hosting a game.
   */
  void connectToNothing() throw();
  /**
   * Initiates a game using the discovery result at the given index.
   * Begins listening automatically.
   */
  void connectToDiscovery(unsigned) throw();
  /**
   * Initiates a game to the given LAN IP address/port combination.
   */
  void connectToLan(const char*, unsigned) throw();
  /**
   * Initiates a game to the given la:lp/ia:ip string (ie, an Internet game).
   */
  void connectToInternet(const char*) throw();

  /**
   * Begins sending UDP hole-punching packets to abendstern.servegame.com.
   *
   * On success for a certain protocol, the GlobalID for the appropriate IP
   * version in the primary antenna is updated appropriately.
   *
   * @param lanpatch true if the "LAN-patch" is active; that is, a set of rule
   * changes that allow networking to function correctly behind the same NAT as
   * the abendstern.servegame.com server.
   */
  void startUdpHolePunch(bool lanpatch) throw();
  /**
   * Returns whether IPv4 hole-punching was successful.
   */
  bool hasInternet4() const throw();
  /**
   * Returns whether IPv6 hole-punching was successful.
   */
  bool hasInternet6() const throw();

  /**
   * Updates the NetworkAssembly and anything else that needs updating.
   */
  void update(unsigned) throw();
};

#endif /* NETWORK_GAME_HXX_ */
