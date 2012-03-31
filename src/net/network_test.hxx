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
#include "network_game.hxx"

class NetworkAssembly;
class HumanController;

class NetworkTest: public TestState, private NetIface {
  NetworkGame game;

public:
  NetworkTest();
  virtual ~NetworkTest();

  virtual GameState* update(float);

private:
  static test_state::Background init();

  virtual void addPeer(Peer*);
  virtual void delPeer(Peer*) {}
  virtual void setOverseer(Peer*) {}
  virtual void receiveBroadcast(Peer*, const char*) {}
  virtual void receiveOverseer(Peer*, const char*) {}
  virtual void receiveUnicast(Peer*, const char*) {}
  virtual bool alterDatp(Peer*, const char*) { return true; }
  virtual bool alterDats(const char*) { return true; }
  virtual void setGameMode(const char*) {}
  virtual const char* getGameMode() { return "TEST"; }
  virtual void connectionLost(const char*) {}
};

#endif /* NETWORK_TEST_HXX_ */
