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
#include <set>
#include <vector>

#include "src/core/aobject.hxx"
#include "globalid.hxx"
#include "network_assembly.hxx"
#include "antenna.hxx"

class NetworkConnection;
class GameAdvertiser;
class GameDiscoverer;
class GameField;

#ifndef DOXYGEN
namespace network_game {
  class NGConnectionListener;
  class NGSeqTextGeraet;
  class PeerConnectivityGeraet;
}
#endif /* DOXYGEN */

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
  ///Whether an STX packet has been received and accepted
  bool receivedStx;

  ///The Peers this Peer has a connection FROM
  std::set<Peer*> connectionsFrom;
};

/**
 * Abstract class used to relay events to the Tcl game system.
 */
class NetIface: public virtual AObject {
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
  virtual bool alterDatp(Peer*, const char* kv) = 0;
  ///Called when a dats alteration is received from the current overseer.
  ///Return true if the changes arv accepted.
  virtual bool alterDats(const char* kv) = 0;

  ///Called when a game mode alteration is received
  virtual void setGameMode(const char*) = 0;
  ///Returns the current game mode string; the first four characters must be a
  ///string appropriate for the game advertiser.
  virtual const char* getGameMode() = 0;

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
  friend class network_game::NGConnectionListener;
  friend class network_game::NGSeqTextGeraet;
  friend class network_game::PeerConnectivityGeraet;

  //These must be above the assembly so that they are destroyed later!
  //All sequential (output) text Geräte associated with this NetworkGame
  typedef std::map<NetworkConnection*,network_game::NGSeqTextGeraet*> stgs_t;
  stgs_t stgs;
  //Same, but for PeerConnectivityGeraete
  typedef std::map<NetworkConnection*,network_game::PeerConnectivityGeraet*>
          pcgs_t;
  pcgs_t pcgs;

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
  NetIface* iface;

  GameAdvertiser* advertiser;
  GameDiscoverer* discoverer;
  network_game::NGConnectionListener* listener;

  unsigned timeSinceSpuriousPCGQuery;

  //If true, operating in LAN-only mode; only the LAN address/port pairs will
  //be used, and Internet pairs ignored. If false, the Internet addresses are
  //used unless rules would indicate to use the LAN address pairs instead.
  bool lanMode;

public:
  /**
   * Constructs a NetworkGame with the given initial GameField.
   */
  NetworkGame(GameField*);

  virtual ~NetworkGame();
  
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
   */
  std::string getDiscoveryResults() throw();

  /**
   * Begins listining for new connections. This is used to start hosting a game.
   *
   * @param ipv6 true if IPv6 should be used, false for IPv4
   * @param lanMode true if a LAN-only game, false if an Internet game
   */
  void connectToNothing(bool ipv6, bool lanMode) throw();
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

  /**
   * Sends the given dats alteration string to the given Peer.
   */
  void alterDats(const std::string&, Peer*) throw();

private:
  bool acceptConnection(const Antenna::endpoint& source,
                        std::string&, std::string&,
                        const std::vector<byte>&)
  throw();

  void peerIsOverseerReady(Peer*) throw();

  void receivePCGDeclaration(Peer*, const GlobalID&, bool positive) throw();
  void receivePCGQuery(Peer*, const GlobalID&) throw();
  void receivePCGGeneralQuery(Peer*) throw();

  //Initialises the ConnectionListener for the given IP version.
  void initialiseListener(bool ipv6) throw();
  //Converts an Asio endpoint to a LAN-only GID
  static void endpointToLanGid(GlobalID&, const asio::ip::udp::endpoint&)
  throw();
  //Creates a Peer with the given endpoint, and then connects to it
  //(with connectToPeer())
  Peer* createPeer(const asio::ip::udp::endpoint&) throw();
  //Creates a Peer with the given already-existing NetworkConnection
  Peer* createPeer(NetworkConnection* cxn) throw();
  //Creates a Peer with the given GlobalID, and then connects to it (with
  //connectToPeer()).
  Peer* createPeer(const GlobalID&) throw();
  //Establishes a new NetworkConnection to the given Peer.
  void connectToPeer(Peer*) throw();

  //Rescans the current Peers to find out who is the current overseer.
  void refreshOverseer() throw();
  //Returns a current Peer associated with the given GlobalID, or NULL if none
  //exists.
  Peer* getPeerByGid(const GlobalID&) throw();

  //Marks the local peer as overseer-ready and notifies other peers of the
  //change
  void becomeOverseerReady() throw();
  
  //Closes any existing connection to the given Peer, removes references to it,
  //sends disconnect notifications to other peers, and may take measures to
  //block reconnects from the peer.
  //Does not delete the Peer.
  void closePeer(Peer*, unsigned banLengthMs=0, bool closeConnection = true)
  throw();

  //Initialises the given Connection with any needed Geräte, and performs any
  //other needed initial actions.
  void initCxn(NetworkConnection*, Peer*) throw();

  //Interprets the STX auxilliary data given; if it is acceptable, sets fields
  //in the Peer; otherwise, closes the peer.
  void acceptStxAux(const std::vector<byte>&, Peer*) throw();
};

#endif /* NETWORK_GAME_HXX_ */
