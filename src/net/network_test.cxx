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

NetworkTest::NetworkTest(const char* host, unsigned port)
: TestState(init()),
  assembly(new NetworkAssembly(env.getField(), &antenna))
{
  //TODO
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
  return TestState::update(et);
}
