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
#include "text_message_geraet.hxx"
#include "src/core/lxn.hxx"
#include "src/core/black_box.hxx"

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
      return game->acceptConnection(source, errmsg, errl10n, auxData);
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
            game->iface->alterDats(extension.c_str());
          break;

        case 'p':
          if (game->iface)
            game->iface->alterDatp(game->peers[cxn], extension.c_str());
          break;

        case 'M':
          if (game->iface && game->overseer == game->peers[cxn])
            game->iface->setGameMode(extension.c_str());
          game->becomeOverseerReady();
          break;

        case 'Q':
          if (game->iface)
            sendDats(game->iface->getFullDats());
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
    void sendQuery() throw() {
      SeqTextOutputGeraet::send(string("Q"));
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

  public:
    ///Size of GlobalID data for IPv4
    static const unsigned ipv4size = 2*(4+2);
    ///Size of GlobalID data for IPv6
    static const unsigned ipv6size = 2*(8*2+2);

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

  public:
    static unsigned writeGid(byte* dst, const GlobalID& gid) throw() {
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

    static void readGid(GlobalID& gid, const byte* src, bool ipv4) throw() {
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

using network_game::PeerConnectivityGeraet;

NetworkGame::NetworkGame(GameField* field)
: overseer(NULL), assembly(field, &antenna),
  iface(NULL), advertiser(NULL), discoverer(NULL), listener(NULL),
  timeSinceSpuriousPCGQuery(0)
{
  localPeer.overseerReady=false;
  localPeer.connectionAttempts=0;
  localPeer.cxn = NULL;
}

NetworkGame::~NetworkGame() {
  BlackBox _bb("netg\0", "%p->~NetworkGame()", this);
  if (advertiser)
    delete advertiser;
  if (discoverer)
    delete discoverer;
  if (listener)
    delete listener;

  //Close all open connections
  while (!peers.empty()) {
    Peer* peer = peers.begin()->second;
    closePeer(peer);
    delete peer;
  }
}

void NetworkGame::setNetIface(NetIface* ifc) throw() {
  iface = ifc;
  for (unsigned i = 0; i < assembly.numConnections(); ++i)
    assembly.getConnection(i)->netiface = ifc;
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
    return -1;
}

bool NetworkGame::discoveryScanDone() const throw() {
  return discoverer && 1.0f == discoverer->progress();
}

string NetworkGame::getDiscoveryResults() throw() {
  ostringstream oss;
  if (!discoverer) return oss.str();

  const vector<GameDiscoverer::Result>& results = discoverer->getResults();
  for (unsigned i=0; i<results.size(); ++i) {
    char gameMode[5] = {0}, buffer[256];
    string realGameMode;
    string ipa(results[i].peer.address().to_string());
    //Get NTBS from game mode
    strncpy(gameMode, results[i].gameMode, 4);
    realGameMode = l10n::lookup('N', "modes", gameMode);
    //Reject if the result contains a #
    if (string::npos != realGameMode.find('#'))
      continue; //Mode not recognised

    sprintf(buffer, "{%4s %02d %c %s} ",
            realGameMode.c_str(), results[i].peerCount,
            results[i].passwordProtected? '*' : ' ',
            ipa.c_str());
    oss << buffer;
  }

  return oss.str();
}

void NetworkGame::setLocalPeerName(const char* name) throw() {
  localPeer.screenName = name;
}

void NetworkGame::setLocalPeerNID(unsigned nid) throw() {
  localPeer.nid = nid;
}

void NetworkGame::setLocalPeerNIDAuto() throw() {
  if (antenna.hasV4()) {
    const GlobalID& gid(*antenna.getGlobalID4());
    localPeer.nid = 0;
    for (int i=0; i < 4; ++i) {
      localPeer.nid <<= 8;
      localPeer.nid |= gid.la4[i];
    }
    localPeer.nid ^= ((unsigned)gid.lport) << 16;
  } else if (antenna.hasV6()) {
    const GlobalID& gid(*antenna.getGlobalID6());
    localPeer.nid = 0;
    for (int i=0; i < 8; ++i) {
      unsigned v = gid.la6[i];
      if (i&1) v <<= 16;
      localPeer.nid ^= v;
    }
    localPeer.nid ^= gid.lport;
  }
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

void NetworkGame::connectToDiscovery(unsigned ix) throw() {
  asio::ip::udp::endpoint endpoint = discoverer->getResults()[ix].peer;
  initialiseListener(endpoint.address().is_v6());
  lanMode = true;
  createPeer(endpoint);
}

void NetworkGame::alterDats(const string& msg, Peer* peer) throw() {
  if (!peer) {
    for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
      alterDats(msg, it->second);
  } else {
    stgs[peer->cxn]->sendDats(msg);
  }
}

void NetworkGame::alterDatp(const string& msg, Peer* peer) throw() {
  if (!peer) {
    for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
      alterDatp(msg, it->second);
  } else {
    stgs[peer->cxn]->sendDatp(msg);
  }
}

void NetworkGame::sendUnicast(const std::string& msg, Peer* peer) throw() {
  stgs[peer->cxn]->sendUnicast(msg);
}

void NetworkGame::sendOverseer(const std::string& msg, Peer* peer) throw() {
  stgs[peer->cxn]->sendOverseer(msg);
}

void NetworkGame::sendBroadcast(const std::string& msg) throw() {
  for (stgs_t::const_iterator it = stgs.begin(); it != stgs.end(); ++it)
    it->second->sendBroadcast(msg);
}
void NetworkGame::sendGameMode(Peer* peer) throw() {
  if (iface)
    stgs[peer->cxn]->sendMode(iface->getGameMode());
}

void NetworkGame::setBlameMask(Peer* peer, unsigned mask) throw() {
  peer->cxn->blameMask = peer->blameMask = mask;
}

void NetworkGame::updateFieldSize() throw() {
  assembly.setFieldSize(assembly.field->width, assembly.field->height);
}

Peer* NetworkGame::getPeerByConnection(NetworkConnection* cxn) throw() {
  assert(peers.count(cxn));
  return peers[cxn];
}

Peer* NetworkGame::getPeerByNid(unsigned nid) throw() {
  if (nid == localPeer.nid) return &localPeer;

  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
    if (it->second->nid == nid) return it->second;

  return NULL;
}

bool NetworkGame::acceptConnection(const Antenna::endpoint& source_,
                                   string& errmsg, string& errl10n,
                                   const std::vector<byte>& auxData)
throw() {
  Antenna::endpoint source(source_);
  BlackBox _bb("netg\0", "NetworkGame %p->acceptConnection(%s:%d)",
               this, source.address().to_string().c_str(), source.port());
  Antenna::endpoint defaultEndpoint;
  if (source == defaultEndpoint) {
    // This is comming via abuhops triangular routing.
    // Get the (Internet) IP address out of the STX aux
    if (getEndpointFromStxAux(source, auxData)) {
      BlackBox _bb("netg\0", "(Resolved address from STX: %s:%d)",
                   source.address().to_string().c_str(), source.port());
    } else {
#ifdef DEBUG
      cerr << "WARN: Unable to get endpoint of triangularly-routed STX" << endl;
#endif
      return false;
    }
  }
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
  Peer* peer = createPeer(cxn);
  acceptStxAux(auxData, peer);
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
  BlackBox _bb("netg\0", "NetworkGame %p->receivePCGDeclaration(%p, %s, %d),%p",
               this, peer, gid.toString().c_str(), (int)positive, referred);
  //Ignore if they are talking about us
  if (referred == &localPeer)
    return;
  if (positive) {
    if (referred)
      peer->connectionsFrom.insert(referred);
    else if (overseer == peer || !localPeer.overseerReady)
      //Newly discovered peer
      peer->connectionsFrom.insert(createPeer(gid));
  } else {
    if (referred) {
      //If this is comming from the overseer, take it as an instruction to
      //disconnect. Otherwise, just record the lost connection.
      if (overseer == peer) {
        closePeer(referred, 15000); //15-second "ban" to prevent reconnects
        delete referred;
      } else {
        peer->connectionsFrom.erase(referred);
      }
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
    io::a6tohbo(gid.la6, &data[0]);
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
  BlackBox _bb("netg\0", "NetworkGame %p->createPeer(GID %s)",
               this, gid.toString().c_str());
  Peer* peer = new Peer;
  peer->gid = gid;
  peer->overseerReady = false;
  peer->connectionAttempts = 0;
  peer->cxn = NULL;
  peer->receivedStx = false;
  connectToPeer(peer);
  if (iface)
    iface->addPeer(peer);
  return peer;
}

Peer* NetworkGame::createPeer(NetworkConnection* cxn) throw() {
  BlackBox _bb("netg\0", "NetworkGame %p->createPeer(CXN %p)",
               this, cxn);
  Peer* peer = new Peer;
  peer->overseerReady = false;
  peer->connectionAttempts = 0;
  peer->cxn = cxn;
  peers[cxn] = peer;
  peer->receivedStx = true;
  initCxn(cxn, peer);
  if (iface)
    iface->addPeer(peer);
  return peer;
}

void NetworkGame::connectToPeer(Peer* peer) throw() {
  BlackBox _bb("netg\0", "NetworkGame %p->connectToPeer(%p)",
               this, peer);
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

    asio::ip::address_v6::bytes_type ba;
    io::a6fromhbo(&ba[0], abytes);
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
  BlackBox _bb("netg\0", "NetworkGame %p->closePeer(%p, %d, %d)",
               this, peer, banLength, (int)closeCxn);
  if (closeCxn)
    peer->cxn->scg->closeConnection();
  assembly.removeConnection(peer->cxn);
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
  if (iface)
    iface->delPeer(peer);
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
  if (localPeer.nid < minid && localPeer.overseerReady)
    os = NULL;

  if (os != overseer) {
    overseer = os;
    if (iface)
      iface->setOverseer(os);
    if (overseer)
      stgs[overseer->cxn]->sendQuery();
  }
  cout << "Overseer: " << overseer;
  if (overseer)
    cout << " (at " << overseer->cxn->endpoint << ")";
  cout << endl;
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
  BlackBox _bb("netg\0", "NetworkGame %p->initCxn(%p, %p)",
               this, cxn, peer);
  cout << "Init cxn: " << cxn->endpoint << endl;
  assembly.addConnection(cxn);
  cxn->scg->openChannel(new network_game::NGSeqTextGeraet(this, cxn),
                        network_game::NGSeqTextGeraet::num);
  cxn->scg->openChannel(new network_game::PeerConnectivityGeraet(this, cxn),
                        network_game::PeerConnectivityGeraet::num);
  cxn->scg->openChannel(new TextMessageOutputGeraet(cxn->aag),
                        TextMessageInputGeraet::num);
  //Send general query to peer to find out who it is connected to.
  //First, clear the list since we'll be getting a full list.
  peer->connectionsFrom.clear();
  pcgs[cxn]->sendGeneralQuery();
  //If overseer-ready, notify them
  if (localPeer.overseerReady)
    stgs[cxn]->sendReady();

  //Send dats info
  if (iface && !overseer && localPeer.overseerReady)
    stgs[cxn]->sendDats(iface->getFullDats());

  //Indicate game mode if appropriate
  if (iface && !overseer && localPeer.overseerReady)
    stgs[cxn]->sendMode(iface->getGameMode());

  //Copy blame mask and netiface
  cxn->blameMask = peer->blameMask;
  cxn->netiface = iface;

  //Set outgoing STX auxilliary data
  byte auxdat[1024];
  byte* auxout = auxdat;
  io::write(auxout, localPeer.nid);
  auxout += PeerConnectivityGeraet::writeGid(auxout, localPeer.gid);
  strcpy((char*)auxout, localPeer.screenName.c_str());
  auxout += localPeer.screenName.size(); //Omits terminating NUL
  cxn->scg->setAuxDataOut(auxdat, auxout);
}

void NetworkGame::becomeOverseerReady() throw() {
  if (!localPeer.overseerReady) {
    localPeer.overseerReady = true;
    for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
      stgs[it->first]->sendReady();
    refreshOverseer();
  }
}

void NetworkGame::acceptStxAux(const std::vector<byte>& auxData, Peer* peer)
throw() {
  //Force the compiler to inline these, since it wants to extenralise them for
  //some reason
  static char ipv4[PeerConnectivityGeraet::ipv4size],
              ipv6[PeerConnectivityGeraet::ipv6size];
  unsigned gidlen = (localPeer.gid.ipv == GlobalID::IPv4?
                     sizeof(ipv4) : sizeof(ipv6));
  if (auxData.size() < 4 + gidlen + 1) {
    //Too short
    closePeer(peer);
    delete peer;
    return;
  }

  const byte* dat = &auxData[0];
  io::read(dat, peer->nid);
  PeerConnectivityGeraet::readGid(peer->gid, dat,
                                  localPeer.gid.ipv == GlobalID::IPv4);
  dat += gidlen;
  peer->screenName.assign((const char*)dat, auxData.size() - 4 - gidlen);
  peer->receivedStx = true;
}

bool NetworkGame::getEndpointFromStxAux(Antenna::endpoint& endpoint,
                                        const vector<byte>& auxData)
throw() {
  //Force the compiler to inline these, since it wants to extenralise them for
  //some reason
  static char ipv4[PeerConnectivityGeraet::ipv4size],
              ipv6[PeerConnectivityGeraet::ipv6size];
  unsigned gidlen = (localPeer.gid.ipv == GlobalID::IPv4?
                     sizeof(ipv4) : sizeof(ipv6));
  if (auxData.size() < 4 + gidlen + 1) {
    //Too short
    return false;
  }

  const byte* dat = &auxData[0];
  dat += 4;
  GlobalID gid;
  PeerConnectivityGeraet::readGid(gid, dat,
                                  localPeer.gid.ipv == GlobalID::IPv4);

  if (localPeer.gid.ipv == GlobalID::IPv4) {
    asio::ip::address_v4::bytes_type addr;
    copy(&addr[0], gid.ia4, &addr[0]+4);
    endpoint = Antenna::endpoint(asio::ip::address(asio::ip::address_v4(addr)),
                                 gid.iport);
  } else {
    asio::ip::address_v6::bytes_type addr;
    io::a6fromhbo(&addr[0], gid.ia6);
    endpoint = Antenna::endpoint(asio::ip::address(asio::ip::address_v6(addr)),
                                 gid.iport);
  }
  return true;
}

void NetworkGame::update(unsigned et) throw() {
  BlackBox _bb("netg\0", "NetworkGame %p->update(%d)", this, et);
  assembly.update(et);
  if (discoverer)
    discoverer->poll(&antenna);

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

  //Ensure that all Established peers have valid STX auxilliary data
  {
    //Copy since the acceptStxAux() function may modify peers
    peers_t peers(this->peers);
    for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it)
      if (it->first->getStatus() == NetworkConnection::Established
      &&  !it->second->receivedStx)
        acceptStxAux(it->first->scg->getAuxData(), it->second);
  }

  //Promote overseer-ready Peers whose connections are Established to Ready
  for (peers_t::const_iterator it = peers.begin(); it != peers.end(); ++it) {
    if (it->first->getStatus() == NetworkConnection::Established
    &&  it->second->overseerReady) {
      it->first->setReady();
    }
  }

  timeSinceSpuriousPCGQuery += et;
  if (timeSinceSpuriousPCGQuery > 1024) {
    timeSinceSpuriousPCGQuery = 0;
    //25% chance: send general query to the overseer (only if we aren't
    //overseer).
    if (overseer && (rand()&3) == 0) {
      pcgs[overseer->cxn]->sendGeneralQuery();
    } else if (!peers.empty()) {
      //Pick a random peer to query
      unsigned ix = rand() % assembly.numConnections();
      Peer* peer = peers[assembly.getConnection(ix)];
      //Pick a random peer we are connected to and send both a positive
      //declaration with a specific query
      ix = rand() % assembly.numConnections();
      Peer* other = peers[assembly.getConnection(ix)];
      pcgs[peer->cxn]->sendPacket(
        network_game::PeerConnectivityGeraet::positiveDec,
        other->gid);
      pcgs[peer->cxn]->sendPacket(
        network_game::PeerConnectivityGeraet::specificQuery,
        other->gid);
    }
  }

  //TODO: Kick peers that are chronically missing connections

  //Maintain the advertiser, if present.
  if (advertiser) {
    advertiser->setPeerCount(peers.size()+1);
    advertiser->setOverseerId(overseer? overseer->nid : localPeer.nid);
    if (iface)
      advertiser->setGameMode(iface->getGameMode());
  }
}
