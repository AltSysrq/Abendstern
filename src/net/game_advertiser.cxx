/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.06
 * @brief Implementation of the GameAdvertiser class
 */

#include <cstring>
#include <iostream>

#include "game_advertiser.hxx"
#include "antenna.hxx"
#include "tuner.hxx"

using namespace std;

static const byte trigger[] = {'A', 'b', 'e', 'n', 'd', 's', 'u', 'c', 'h'};

GameAdvertiser::GameAdvertiser(Tuner* tuner_,
                               bool v6_,
                               Uint32 overseerid_,
                               byte peerCount_,
                               bool passwordProtected_,
                               const char* gameMode_)
: tuner(tuner_), v6(v6_),
  overseerid(overseerid_),
  peerCount(peerCount_),
  passwordProtected(passwordProtected_)
{
  setGameMode(gameMode_);
  tuner->trigger(trigger, sizeof(trigger), this);
}

GameAdvertiser::~GameAdvertiser() {
  tuner->untrigger(trigger, sizeof(trigger));
}

void GameAdvertiser::setOverseerId(Uint32 oid) {
  overseerid = oid;
}

void GameAdvertiser::setPeerCount(byte cnt) {
  peerCount = cnt;
}

void GameAdvertiser::setGameMode(const char* mode) {
  strncpy(gameMode, mode, 4);
}

void GameAdvertiser::process(const Antenna::endpoint& source,
                             Antenna* antenna, Tuner* tuner,
                             const byte* data, unsigned len)
noth {
  //Ignore if it is the wrong protocol
  if (v6 == (source.protocol() == asio::ip::udp::v6())) {
    //OK
    byte pack[] = {
      'A', 'b', 'e', 'n', 'd', 's', 'p', 'i', 'e', 'l',
      (byte)(overseerid >> 24),
      (byte)(overseerid >> 16),
      (byte)(overseerid >>  8),
      (byte)(overseerid >>  0),
      peerCount, passwordProtected,
      (byte)gameMode[0], (byte)gameMode[1],
      (byte)gameMode[2], (byte)gameMode[3],
    };
    try {
      antenna->send(source, pack, sizeof(pack));
    } catch (asio::system_error e) {
      cerr << "Warning: Unable to respond to Abendsuch from " << source
           << ": " << e.what() << endl;
    }
  }
}
