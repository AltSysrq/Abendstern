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

NetworkTest::NetworkTest()
: TestState(init()),
  game(&env.field)
{
  game.setNetIface(this);
  //Connect to the first running game found, or start a new one otherwise
  game.startDiscoveryScan();
  while (!game.discoveryScanDone()) {
    SDL_Delay(5);
    game.update(5);
  }

  //If any result was found, connect to it
  if (!game.getDiscoveryResults(false).empty())
    game.connectToDiscovery(0, false);
  else
    //Otherwise, start IPv4 LAN game
    game.connectToNothing(false, true);

  game.setAdvertising("TEST");
}

NetworkTest::~NetworkTest() {
}

test_state::Background NetworkTest::init() {
  test_state::gameClass = "effective";
  test_state::mode = test_state::FreeForAll;
  test_state::size = 32;
  return test_state::StarField;
}

GameState* NetworkTest::update(float et) {
  game.update((unsigned)et);
  return TestState::update(et);
}

void NetworkTest::addPeer(Peer* peer) {
  if (!game.getOverseer())
    game.alterDats("", peer);
}
