/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.11
 * @brief Implements the network testing functions
 */

#include <SDL.h>
#include <asio.hpp>

#include "antenna.hxx"
#include "tuner.hxx"
#include "network_assembly.hxx"
#include "network_connection.hxx"
#include "network_geraet.hxx"
#include "connection_listener.hxx"
#include "synchronous_control_geraet.hxx"
#include "io.hxx"
#include "src/sim/game_field.hxx"

unsigned globlcount;

class TestInputGeraet: public InputNetworkGeraet {
public:
  virtual void receive(NetworkConnection::seq_t,
                       const byte* data, unsigned) throw() {
    ++globlcount;
  }

  static InputNetworkGeraet* create(NetworkConnection*) {
    return new TestInputGeraet;
  }
};

class TestOutputGeraet: public OutputNetworkGeraet {
  NetworkConnection* parent;
  signed timeUntilPing;
  unsigned pingsLeft;

public:
  TestOutputGeraet(NetworkConnection* par)
  : parent(par), timeUntilPing(0), pingsLeft(10240)
  { }

  virtual void update(unsigned et) throw() {
    timeUntilPing -= et;
    if (timeUntilPing < 0) {
      byte pack[] = "xxxxHello, world!";
      byte* pptr = pack;
      io::write(pptr, parent->seq());
      io::write(pptr, channel);
      parent->send(pack, sizeof(pack));

      timeUntilPing = 10;
      --pingsLeft;
      if (!pingsLeft) {
        parent->scg->closeConnection();
      }
    }
  }
};

class TestListener: public ConnectionListener {
  NetworkAssembly* nasm;
public:
  TestListener(NetworkAssembly* nas, Tuner* tuner)
  : ConnectionListener(tuner), nasm(nas)
  { }

  virtual bool acceptConnection(const Antenna::endpoint& source,
                                Antenna* antenna, Tuner* tuner,
                                std::string& errmsg,
                                std::string& errl10n)
  noth {
    NetworkConnection* cxn = new NetworkConnection(nasm, source, true);
    nasm->addConnection(cxn);
    cxn->scg->openChannel(new TestOutputGeraet(cxn), 999);
    return true;
  }
};

void networkTestListen() {
  GameField field(1,1);
  NetworkAssembly assembly(&field, &antenna);
  assembly.addPacketProcessor(new TestListener(&assembly, assembly.getTuner()));

  while (assembly.numConnections() == 0
  ||     assembly.getConnection(0)->getStatus() != NetworkConnection::Zombie) {
    assembly.update(5);
    SDL_Delay(5);
  }
}

unsigned networkTestRun(const char* addrstr, unsigned portn) {
  globlcount = 0;
  GameField field(1,1);
  NetworkAssembly assembly(&field, &antenna);
  NetworkConnection::registerGeraetCreator(&TestInputGeraet::create, 999);

  asio::ip::udp::endpoint endpoint(asio::ip::address::from_string(addrstr),
                                   portn);
  assembly.addConnection(new NetworkConnection(&assembly, endpoint, false));
  while (assembly.getConnection(0)->getStatus() != NetworkConnection::Zombie) {
    assembly.update(5);
    SDL_Delay(5);
  }
  return globlcount;
}
