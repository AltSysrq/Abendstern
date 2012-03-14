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
#include <cstdlib>

#include <SDL.h>
#include <asio.hpp>

#include "network_test.hxx"

#include "antenna.hxx"
#include "tuner.hxx"
#include "network_assembly.hxx"
#include "network_connection.hxx"
#include "connection_listener.hxx"
#include "synchronous_control_geraet.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/game_env.hxx"
#include "src/control/human_controller.hxx"
#include "src/control/hc_conf.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/shipio.hxx"
#include "src/test_state.hxx"
#include "src/globals.hxx"

using namespace std;

class NetworkTestListener: public ConnectionListener {
  NetworkAssembly*const assembly;
public:
  NetworkTestListener(NetworkAssembly* nasm)
  : ConnectionListener(nasm->getTuner()),
    assembly(nasm)
  { }

protected:
  virtual bool acceptConnection(const Antenna::endpoint& source,
                                Antenna* antenna, Tuner* tuner,
                                string&, string&)
  noth {
    assembly->addConnection(new NetworkConnection(assembly, source, true));
    return true;
  }
};

NetworkTest::NetworkTest()
: TestState(init()),
  assembly(new NetworkAssembly(env.getField(), &antenna))
{
  new NetworkTestListener(assembly);
}

void NetworkTest::connect(const char* host, unsigned port) {
  asio::ip::udp::endpoint dst(
    asio::ip::address::from_string(host), port);
  assembly->addConnection(new NetworkConnection(assembly, dst, false));
}

NetworkTest::~NetworkTest() {
  delete assembly;
}

test_state::Background NetworkTest::init() {
  test_state::gameClass = "effective";
  test_state::mode = test_state::FreeForAll;
  test_state::size = 32;
  return test_state::StarField;
}

GameState* NetworkTest::update(float et) {
  assembly->update((unsigned)et);
  //Remove dead connections
  for (unsigned i=0; i<assembly->numConnections(); ++i)
    if (NetworkConnection::Zombie == assembly->getConnection(i)->getStatus())
      assembly->removeConnection(i--);
  return TestState::update(et);
}
