/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.11
 * @brief Implements the network testing functions
 */

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <SDL.h>
#include <asio.hpp>

#include "antenna.hxx"
#include "tuner.hxx"
#include "network_assembly.hxx"
#include "network_connection.hxx"
#include "network_geraet.hxx"
#include "connection_listener.hxx"
#include "synchronous_control_geraet.hxx"
#include "seq_text_geraet.hxx"
#include "io.hxx"
#include "src/sim/game_field.hxx"

using namespace std;

static unsigned globlcount;
static vector<string> textin;

class TestInputGeraet: public SeqTextInputGeraet {
public:
  TestInputGeraet(AsyncAckGeraet* aag)
  : SeqTextInputGeraet(aag)
  { }

  virtual void receiveText(string str) noth {
    textin.push_back(str);
    ++globlcount;
  }

  static InputNetworkGeraet* create(NetworkConnection* cxn) {
    return new TestInputGeraet(cxn->aag);
  }
};

class TestOutputGeraet: public SeqTextOutputGeraet {
  ifstream input;
  signed timeUntilClose;

public:
  TestOutputGeraet(NetworkConnection* par)
  : SeqTextOutputGeraet(par->aag),
    input("tcl/bridge.tcl"), timeUntilClose(1024)
  { }

  virtual void update(unsigned et) throw() {
    if (input) {
      string line;
      for (unsigned i=0; i < 1 && getline(input, line); ++i) {
        send(line);
      }
    } else {
      if ((timeUntilClose -= et) < 0) {
        cxn->scg->closeConnection();
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
    cxn->scg->openChannel(cxn->aag, cxn->aag->num);
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
  NetworkConnection* cxn = new NetworkConnection(&assembly, endpoint, false);
  assembly.addConnection(cxn);
  cxn->scg->openChannel(cxn->aag, cxn->aag->num);
  while (cxn->getStatus() != NetworkConnection::Zombie) {
    assembly.update(5);
    SDL_Delay(5);
  }

  //Write text received to file
  ofstream out("received.txt");
  for (unsigned i=0; i<textin.size(); ++i)
    out << textin[i] << endl;

  return globlcount;
}
