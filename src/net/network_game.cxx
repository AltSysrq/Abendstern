/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.15
 * @brief Implementation of NetworkGame classes.
 */

#include <map>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <cassert>
#include <algorithm>

#include <asio.hpp>

#include "network_game.hxx"
#include "network_geraet.hxx"
#include "tuner.hxx"
#include "connection_listener.hxx"
#include "seq_text_geraet.hxx"
#include "text_message_geraet.hxx"
#include "async_ack_geraet.hxx"
#include "io.hxx"
#include "globalid.hxx"
#include "game_advertiser.hxx"
#include "game_discoverer.hxx"
#include "synchronous_control_geraet.hxx"

using namespace std;

namespace network_game {
  /* Simple ConnectionListener to forward connection attempts to NetworkGame.
   */
  class NGConnectionListener: public ConnectionListener {
    NetworkGame*const game;
  public:
    NGConnectionListener(NetworkGame* game_)
    : ConnectionListener(game_->assembly.getTuner()),
      game(game_)
    { }

    virtual bool acceptConnection(const Antenna::endpoint& source,
                                  Antenna* antenna, Tuner* tuner,
                                  string& errmsg, string& errl10n)
    noth {
      return game->acceptConnection(source, errmsg, errl10n);
    }
  };

  /* Handles encoding and decoding of sequential text messages, including
   * message types and fragmentation, and provides a symbolic interface to
   * the messaging system.
   */
  class NGSeqTextGeraet: public SeqTextInputGeraet, public SeqTextOutputGeraet {
    //Accumulation of input extensions
    string extension;
    //Allow finding the output side associated with each NetworkConnection
    typedef map<NetworkConnection*,NGSeqTextGeraet*> geraete_t;
    static geraete_t geraete;

    NetworkGame*const game;

  public:
    static const NetworkConnection::geraet_num num;

    NGSeqTextGeraet(NetworkGame* game_, NetworkConnection* cxn)
    : SeqTextInputGeraet(cxn->aag, InputNetworkGeraet::DSEternal),
      SeqTextOutputGeraet(cxn->aag, OutputNetworkGeraet::DSBidir),
      game(game_)
    {
      geraete[cxn] = this;
      game->stgs[cxn] = this;
    }

    virtual ~NGSeqTextGeraet() {
      geraete.erase(cxn);
      game->stgs.erase(cxn);
    }

    virtual void receiveText(string txt) noth {
      if (txt.empty()) {
        #ifdef DEBUG
        cerr << "Warning: Ignoring empty sequential text message" << endl;
        #endif
        return;
      }

      const char* body = txt.c_str()+1;
      //Don't let the remote system fill up memory with extensions
      if (extension.size() < 65536)
        extension += body;
      else {
        #ifdef DEBUG
        cerr << "Warning: STG ext dropped due to extension size"<< endl;
        #endif
      }
      switch (txt[0]) {
        case 'x':
          return; //Nothing further to do, and don't clear extension

        case 'o':
          if (game->iface)
            game->iface->receiveOverseer(game->peers[cxn], extension.c_str());
          break;

        case 'u':
          if (game->iface)
            game->iface->receiveUnicast(game->peers[cxn], extension.c_str());
          break;

        case 'b':
          if (game->iface)
            game->iface->receiveBroadcast(game->peers[cxn], extension.c_str());
          break;

        case 's':
          if (game->iface && game->overseer == game->peers[cxn])
            if (game->iface->alterDats(extension.c_str()))
              game->becomeOverseerReady();
          break;

        case 'p':
          if (game->iface)
            game->iface->alterDatp(game->peers[cxn], extension.c_str());
          break;

        case 'M':
          if (game->iface && game->overseer == game->peers[cxn])
            game->iface->setGameMode(extension.c_str());
          break;

        case 'V':
          //TODO
          break;

        case 'R':
          game->peerIsOverseerReady(game->peers[cxn]);
          break;

        default:
          #ifdef DEBUG
          cerr << "Warning: Ignoring unknown STG message: " << txt[0] << endl;
          #endif
          break;
      }

      //Clear accumulated text
      extension.clear();
    }

    void sendOverseer(const std::string& str) throw() {
      send(str, 'o');
    }
    void sendUnicast(const std::string& str) throw() {
      send(str, 'u');
    }
    void sendBroadcast(const std::string& str) throw() {
      send(str, 'b');
    }
    void sendDats(const std::string& str) throw() {
      send(str, 's');
    }
    void sendDatp(const std::string& str) throw() {
      send(str, 'p');
    }
    void sendMode(const std::string& str) throw() {
      send(str, 'M');
    }
    void sendVote(const std::string& str) throw() {
      send(str, 'V');
    }
    void sendReady() throw() {
      SeqTextOutputGeraet::send(string("R"));
    }

  private:
    void send(const std::string& str, char type) throw() {
      static const unsigned segsz = 250;
      for (unsigned i=0; i<str.size(); i += segsz) {
        //Only the last (where i+segsz >= str.size()) gets the actual type;
        //other fragments have type x for extension.
        string msg(1, i+segsz < str.size()? 'x' : type);
        msg += str.substr(i, segsz);
        SeqTextOutputGeraet::send(msg);
      }
    }

    static InputNetworkGeraet* create(NetworkConnection* cxn) {
      geraete_t::iterator it = geraete.find(cxn);
      if (it == geraete.end()) return NULL;

      InputNetworkGeraet* ong = it->second;
      geraete.erase(it);
      return ong;
    }
  };
  NGSeqTextGeraet::geraete_t NGSeqTextGeraet::geraete;
  const NetworkConnection::geraet_num NGSeqTextGeraet::num =
    NetworkConnection::registerGeraetCreator(&create, 2);

  /* Handles both sides of the PeerConnectivityGeraet (see documentation in
   * newnetworking.txt).
   *
   * Since this has a strong dependency on NetworkGame, it is not placed in
   * a separate header/implementation file pair.
   */
  class PeerConnectivityGeraet: public AAGReceiver, public ReliableSender {
    //Map NetworkConnection*s to unopened input sides.
    typedef map<NetworkConnection*,PeerConnectivityGeraet*> geraete_t;
    static geraete_t geraete;

    NetworkGame*const game;

    //Sizes of GlobalID data for IPv4 and IPv6, respectively
    static const unsigned ipv4size = 2*(4+2);
    static const unsigned ipv6size = 2*(8*2+2);

  public:
    static const unsigned positiveDec = 0,
                          negativeDec = 1,
                          specificQuery = 2,
                          generalQuery = 3;

    static const NetworkConnection::geraet_num num;

    PeerConnectivityGeraet(NetworkGame* game_, NetworkConnection* cxn)
    : AAGReceiver(cxn->aag, InputNetworkGeraet::DSEternal),
      ReliableSender(cxn->aag, OutputNetworkGeraet::DSBidir),
      game(game_)
    {
      geraete[cxn] = this;
      game->pcgs[cxn] = this;
    }

    virtual ~PeerConnectivityGeraet() {
      geraete.erase(cxn);
      game->pcgs.erase(cxn);
    }

    void sendPacket(unsigned type, const GlobalID& gid) throw() {
      byte pack[NetworkConnection::headerSize + 1 + ipv6size];
      byte* dst = pack+NetworkConnection::headerSize;
      *dst++ = type;
      unsigned gidlen = writeGid(dst, gid);
      send(pack, NetworkConnection::headerSize + 1 + gidlen);
    }

    void sendGeneralQuery() throw() {
      byte pack[NetworkConnection::headerSize + 1];
      pack[NetworkConnection::headerSize] = generalQuery;
      send(pack, sizeof(pack));
    }

  protected:
    virtual void receiveAccepted(NetworkConnection::seq_t seq, const byte* data,
                                 unsigned len)
    throw() {
      if (!len) {
        #ifdef DEBUG
        cerr << "Warning: Dropping empty packet to Peer Connectivity Geraet."
             << endl;
        #endif
        return;
      }

      GlobalID gid;
      unsigned type = data[0];
      ++data, --len;

      switch (type) {
        case positiveDec:
        case negativeDec:
          if (len == ipv4size)
            readGid(gid, data, true);
          else if (len == ipv6size)
            readGid(gid, data, false);
          else {
            #ifdef DEBUG
            cerr << "Warning: Dropping packet with bad length to Peer"
                 << "Connectivity Geraet" << endl;
            #endif
            return;
          }

          game->receivePCGDeclaration(game->peers[cxn], gid,
                                      type == positiveDec);
          break;

        case specificQuery:
          if (len == ipv4size)
            readGid(gid, data, true);
          else if (len == ipv6size)
            readGid(gid, data, false);
          else {
            #ifdef DEBUG
            cerr << "Warning: Dropping packet with bad length to Peer"
                 << "Connectivity Geraet" << endl;
            #endif
            return;
          }

          game->receivePCGQuery(game->peers[cxn], gid);
          break;

        case generalQuery:
          game->receivePCGGeneralQuery(game->peers[cxn]);
          break;

        default:
          #ifdef DEBUG
          cerr << "Warning: Dropping packet of unknown type "
               << type << " to Peer Connectivity Geraet" << endl;
          #endif
          return;
      }
      
    }

  private:
    static InputNetworkGeraet* create(NetworkConnection* cxn) {
      geraete_t::iterator it = geraete.find(cxn);
      if (it == geraete.end()) return NULL;

      InputNetworkGeraet* ing = it->second;
      geraete.erase(it);
      return ing;
    }

    unsigned writeGid(byte* dst, const GlobalID& gid) const throw() {
      if (gid.ipv == GlobalID::IPv4) {
        memcpy(dst, gid.ia4, 4);
        dst += 4;
        io::write(dst, gid.iport);
        memcpy(dst, gid.la4, 4);
        dst += 4;
        io::write(dst, gid.lport);
        return ipv4size;
      } else {
        for (unsigned i=0; i<8; ++i)
          io::write(dst, gid.ia6[i]);
        io::write(dst, gid.iport);
        for (unsigned i=0; i<8; ++i)
          io::write(dst, gid.la6[i]);
        io::write(dst, gid.lport);
        return ipv6size;
      }
    }

    void readGid(GlobalID& gid, const byte* src, bool ipv4) const throw() {
      if (ipv4) {
        memcpy(gid.ia4, src, 4);
        src += 4;
        io::read(src, gid.iport);
        memcpy(gid.la4, src, 4);
        src += 4;
        io::read(src, gid.lport);
        gid.ipv = GlobalID::IPv4;
      } else {
        for (unsigned i=0; i<8; ++i)
          io::read(src, gid.ia6[i]);
        io::read(src, gid.iport);
        for (unsigned i=0; i<8; ++i)
          io::read(src, gid.la6[i]);
        io::read(src, gid.lport);
        gid.ipv = GlobalID::IPv6;
      }
    }
  };
  PeerConnectivityGeraet::geraete_t PeerConnectivityGeraet::geraete;
  const NetworkConnection::geraet_num PeerConnectivityGeraet::num =
    NetworkConnection::registerGeraetCreator(&create, 5);
}

NetworkGame::NetworkGame(GameField* field)
: overseer(NULL), assembly(field, &antenna),
  iface(NULL), advertiser(NULL), discoverer(NULL),
  timeSinceSpuriousPCGQuery(0)
{
  localPeer.overseerReady=false;
  localPeer.connectionAttempts=0;
  localPeer.cxn = NULL;
}

NetworkGame::~NetworkGame() {
  if (advertiser)
    delete advertiser;
  if (discoverer)
    delete discoverer;
  if (listener)
    delete listener;
}

void NetworkGame::setNetIface(NetIface* ifc) throw() {
  iface = ifc;
}

void NetworkGame::setAdvertising(const char* gameMode) throw() {
  if (advertiser)
    advertiser->setGameMode(gameMode);
  else
    advertiser = new GameAdvertiser(assembly.getTuner(),
                                    localPeer.gid.ipv == GlobalID::IPv6,
                                    overseer? overseer->nid : localPeer.nid,
                                    peers.size()+1, //+1 for self
                                    false, //TODO
                                    gameMode);
}

void NetworkGame::stopAdvertising() throw() {
  if (advertiser) {
    delete advertiser;
    advertiser = NULL;
  }
}

void NetworkGame::startDiscoveryScan() throw() {
  if (!discoverer)
    discoverer = new GameDiscoverer(assembly.getTuner());
  discoverer->start();
}

float NetworkGame::discoveryScanProgress() const throw() {
  if (discoverer)
    return discoverer->progress();
  else
    return 0;
}

bool NetworkGame::discoveryScanDone() const throw() {
  return discoverer && 1.0f == discoverer->progress();
}

string NetworkGame::getDiscoveryResults() throw() {
  ostringstream oss;
  if (!discoverer) return oss.str();

  const vector<GameDiscoverer::Result>& results = discoverer->getResults();
  for (unsigned i=0; i<results.size(); ++i) {
    char gameMode[5], buffer[256];
    string ipa(results[i].peer.address().to_string());
    //Get NTBS from game mode
    strncpy(gameMode, results[i].gameMode, sizeof(gameMode));
    //TODO: localise game mode (doing so will also sanitise it of characters
    //such as \} that would otherwise break the list)
    #ifndef WIN32
    #warning Game mode not localised in NetworkGame::getDiscoveryResults()
    #endif
    sprintf(buffer, "{%4s %02d %c %s} ",
            gameMode, results[i].peerCount,
            results[i].passwordProtected? '*' : ' ',
            ipa.c_str());
    oss << buffer;
  }

  return oss.str();
}

void NetworkGame::connectToNothing(bool v6, bool lanMode) throw() {
  initialiseListener(v6);
  this->lanMode = lanMode;
  //Since we need no data from anyone, we are immediately overseer-ready
  localPeer.overseerReady = true;
}

void NetworkGame::connectToLan(const char* ipaddress, unsigned port) throw() {
  lanMode = true;
  asio::ip::address address;
  //It would be nice if Asio's documentation on from_string actually specified
  //what would happen when the address is invalid (I found this out by trial
  //and error...)
  try {
    address = asio::ip::address::from_string(ipaddress);
  } catch (asio::system_error err) {
    //Invalid address
    lastDisconnectReason = err.what();
    if (iface)
      iface->connectionLost(lastDisconnectReason.c_str());
    return;
  }

  initialiseListener(address.is_v6());
  createPeer(asio::ip::udp::endpoint(address, port));
}

bool NetworkGame::acceptConnection(const Antenna::endpoint& source,
                                   string& errmsg, string& errl10n)
throw() {
  /* If there already exists a NetworkConnection to this endpoint, return true
   * but do nothing else (this could happen if we opened a NetworkConnection
   * before updating the NetworkAssembly).
   *
   * Otherwise, count the number of connections from the given IP address. If
   * it is already 4 or greater, deny the connection.
   *
   * Also deny the connection if the remote host is using a different IP
   * version than we are expecting.
   */
  unsigned addressCount = 0;
  if (source.address().is_v6() != (localPeer.gid.ipv == GlobalID::IPv6)) {
    errmsg = "Wrong IP version";
    errl10n = "wrong_ip_version";
    return false;
  }

  for (unsigned i=0; i < assembly.numConnections(); ++i) {
    const NetworkConnection* cxn = assembly.getConnection(i);
    if (cxn->endpoint == source) {
      //Already have this connection
      return true;
    }

    if (cxn->endpoint.address() == source.address())
      ++addressCount;
  }

  if (addressCount >= 4)
    return false;

  //All checks passed
  NetworkConnection* cxn = new NetworkConnection(&assembly, source, true);
  assembly.addConnection(cxn);
  createPeer(cxn);
  return true;
}

void NetworkGame::peerIsOverseerReady(Peer* peer) throw() {
  peer->overseerReady = true;
  refreshOverseer();
}

void NetworkGame::receivePCGDeclaration(Peer* peer, const GlobalID& gid,
                                        bool positive)
throw() {
  Peer* referred = getPeerByGid(gid);
  if (positive) {
    if (referred)
      peer->connectionsFrom.insert(referred);
    else if (overseer == peer)
      //Newly discovered peer
      peer->connectionsFrom.insert(createPeer(gid));
  } else {
    if (referred) {
      //If this is comming from the overseer, take it as an instruction to
      //disconnect. Otherwise, just record the lost connection.
      if (overseer == peer)
        closePeer(referred, 15000); //15-second "ban" to prevent reconnect attempts
      peer->connectionsFrom.erase(referred);
      delete referred;
    }
    //Else, we know nothing of this peer, so just ignore.
  }
}

void NetworkGame::receivePCGQuery(Peer* peer, const GlobalID& gid)
throw() {
  bool positive = (bool)getPeerByGid(gid);
  //Broadcast answer to all peers
  unsigned type = positive?
    /* G++ for some reason wants to externalise these when used here.
     * But we can't make them proper constants (eg, with effectively extern
     * linkage) because we need them to be usable in constexprs.
     * So we're just substituting the values in by hand for now...
    network_game::PeerConnectivityGeraet::positiveDec :
    network_game::PeerConnectivityGeraet::negativeDec;
    */ 0:1;
  for (pcgs_t::const_iterator it = pcgs.begin(); it != pcgs.end(); ++it)
    it->second->sendPacket(type, gid);
}

void NetworkGame::receivePCGGeneralQuery(Peer* peer) throw() {
  //Send responses to sender
  network_game::PeerConnectivityGeraet* pcg = pcgs[peer->cxn];
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
    pcg->sendPacket(network_game::PeerConnectivityGeraet::positiveDec,
                    it->second->gid);
}

void NetworkGame::initialiseListener(bool ipv6) throw() {
  localPeer.gid = *(ipv6? antenna.getGlobalID6() : antenna.getGlobalID4());
  listener = new network_game::NGConnectionListener(this);
}

void NetworkGame::endpointToLanGid(GlobalID& gid,
                                   const asio::ip::udp::endpoint& endpoint)
throw() {
  const asio::ip::address& address = endpoint.address();
  if (address.is_v4()) {
    gid.ipv = GlobalID::IPv4;
    const asio::ip::address_v4 addr(address.to_v4());
    memcpy(gid.la4, &addr.to_bytes()[0], 4);
  } else {
    gid.ipv = GlobalID::IPv6;
    const asio::ip::address_v6 addr(address.to_v6());
    const asio::ip::address_v6::bytes_type data(addr.to_bytes());
    for (unsigned i=0; i<8; ++i)
      gid.la6[i] = data[i*2] | (data[i*2+1] << 8);
  }
  gid.lport = endpoint.port();
}

Peer* NetworkGame::createPeer(const asio::ip::udp::endpoint& endpoint) throw() {
  //This should only be called in lanMode.
  assert(lanMode);

  //Convert to a GlobalID and connect to that.
  GlobalID gid;
  endpointToLanGid(gid, endpoint);
  return createPeer(gid);
}

Peer* NetworkGame::createPeer(const GlobalID& gid) throw() {
  Peer* peer = new Peer;
  peer->gid = gid;
  peer->overseerReady = false;
  peer->connectionAttempts = 0;
  peer->cxn = NULL;
  connectToPeer(peer);
  return peer;
}

Peer* NetworkGame::createPeer(NetworkConnection* cxn) throw() {
  Peer* peer = new Peer;
  //TODO: For now, just infer the gid from the IP address.
  #ifndef WIN32
  #warning NetworkGame::createPeer(NetworkConnection*) does not honour provided GID
  #endif
  endpointToLanGid(peer->gid, cxn->endpoint);
  //TODO: Numeric ID
  #ifndef WIN32
  #warning NetworkGame::createPeer(NetworkConnection*) does not set NID
  #endif
  peer->overseerReady = false;
  peer->connectionAttempts = 0;
  peer->cxn = cxn;
  peers[cxn] = peer;
  initCxn(cxn, peer);
  return peer;
}

void NetworkGame::connectToPeer(Peer* peer) throw() {
  assert(peer->gid.ipv == localPeer.gid.ipv);
  const GlobalID& pgid(peer->gid), & lgid(localPeer.gid);
  unsigned short port;
  asio::ip::address addr;
  if (pgid.ipv == GlobalID::IPv4) {
    const unsigned char* abytes = (lanMode || !memcmp(pgid.ia4, lgid.ia4,
                                                      sizeof(pgid.ia4))?
                                   pgid.la4 : pgid.ia4);
    port = (lanMode || !memcmp(pgid.ia4, lgid.ia4, sizeof(pgid.ia4))?
            pgid.lport : pgid.iport);
    //Why doesn't boost::array have a constructor taking a native array?
    asio::ip::address_v4::bytes_type ba;
    memcpy(&ba[0], abytes, 4);
    addr = asio::ip::address_v4(ba);
  } else {
    const unsigned short* abytes = (lanMode || !memcmp(pgid.ia6, lgid.ia6,
                                                       sizeof(pgid.ia6))?
                                    pgid.la6 : pgid.ia6);
    port = (lanMode || !memcmp(pgid.ia6, lgid.ia6, sizeof(pgid.ia6))?
            pgid.lport : pgid.iport);
    //Why doesn't boost::array have a constructor taking a native array?
    asio::ip::address_v6::bytes_type ba;
    memcpy(&ba[0], abytes, 16);
    #if SDL_BYTEORDER == SDL_LIL_ENDIAN
    //Convert to NBO
    for (unsigned i = 0; i < 16; ++i)
      swap(ba[i], ba[i+1]);
    #endif
    addr = asio::ip::address_v6(ba);
  }

  asio::ip::udp::endpoint endpoint(addr, port);
  NetworkConnection* cxn = new NetworkConnection(&assembly, endpoint, false);
  peer->cxn = cxn;
  peers[cxn] = peer;
  initCxn(cxn, peer);
}

void NetworkGame::closePeer(Peer* peer, unsigned banLength, bool closeCxn)
throw() {
  if (closeCxn)
    peer->cxn->scg->closeConnection();
  peers.erase(peer->cxn);
  //Remove references and send notifications
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it) {
    Peer* p = it->second;
    p->connectionsFrom.erase(peer);
    pcgs[p->cxn]->sendPacket(network_game::PeerConnectivityGeraet::negativeDec,
                             peer->gid);
  }
  if (peer == overseer)
    refreshOverseer();
  //TODO: handle banning
}

void NetworkGame::refreshOverseer() throw() {
  unsigned minid = -1 /* max value */;
  Peer* os = NULL;

  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it) {
    Peer* p = it->second;
    if (p->overseerReady && (p->nid < minid || !os)) {
      os = p;
      minid = p->nid;
    }
  }

  //Local peer is overseer if it has a lower NID or if no other peer is
  //ready (in which case os is already NULL).
  if (localPeer.nid < minid)
    os = NULL;

  if (os != overseer) {
    overseer = os;
    if (iface)
      iface->setOverseer(os);
  }
}

Peer* NetworkGame::getPeerByGid(const GlobalID& gid) throw() {
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
    if (it->second->gid == gid)
      return it->second;

  //None of the connected peers, but might be self
  if (localPeer.gid == gid)
    return &localPeer;

  //None match
  return NULL;
}

void NetworkGame::initCxn(NetworkConnection* cxn, Peer* peer) throw() {
  cxn->scg->openChannel(new network_game::NGSeqTextGeraet(this, cxn),
                        network_game::NGSeqTextGeraet::num);
  cxn->scg->openChannel(new network_game::PeerConnectivityGeraet(this, cxn),
                        network_game::PeerConnectivityGeraet::num);
  //Send general query to peer to find out who it is connected to.
  //First, clear the list since we'll be getting a full list.
  peer->connectionsFrom.clear();
  pcgs[cxn]->sendGeneralQuery();
  //If overseer-ready, notify them
  if (localPeer.overseerReady)
    stgs[cxn]->sendReady();

  //Indicate game mode if appropriate
  if (iface && !overseer && localPeer.overseerReady)
    stgs[cxn]->sendMode(iface->getGameMode());
}

void NetworkGame::becomeOverseerReady() throw() {
  if (!localPeer.overseerReady) {
    localPeer.overseerReady = true;
    for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
      stgs[it->first]->sendReady();
    refreshOverseer();
  }
}

void NetworkGame::update(unsigned et) throw() {
  assembly.update(et);

  //Reap zombies
  //TODO: Attempt reconnects
  list<Peer*> zombies;
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it) {
    if (it->first->getStatus() == NetworkConnection::Zombie) {
      lastDisconnectReason = it->first->getDisconnectReason();
      zombies.push_back(it->second);
    }
  }
  while (!zombies.empty()) {
    closePeer(zombies.front(), 0, false);
    delete zombies.front();
    zombies.pop_front();
  }

  //Promote overseer-ready Peers whose connections are Established to Ready
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it) {
    if (it->first->getStatus() == NetworkConnection::Established
    &&  it->second->overseerReady) {
      it->first->setReady();
    }
  }

  //TODO: Spurious PCG requests
  //TODO: Kick peers that are chronically missing connections

  //Maintain the advertiser, if present.
  if (advertiser) {
    advertiser->setPeerCount(peers.size()+1);
    advertiser->setOverseerId(overseer? overseer->nid : localPeer.nid);
    if (iface)
      advertiser->setGameMode(iface->getGameMode());
  }
}
