/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.11
 * @brief Contains entry points for testing the networking system.
 *
 * Functions within this file are temporary and undocumnted.
 */
#ifndef NETWORK_TEST_HXX_
#define NETWORK_TEST_HXX_

#include "src/test_state.hxx"

class NetworkAssembly;
class HumanController;

class NetworkTest: public TestState {
  NetworkAssembly* assembly;

public:
  NetworkTest();
  virtual ~NetworkTest();

  virtual GameState* update(float);
  virtual void connect(const char* host, unsigned port);

private:
  static test_state::Background init();
};

#endif /* NETWORK_TEST_HXX_ */
