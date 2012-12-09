/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.06
 * @brief Implementation of the GameAdvertiser class
 */

#include <cstring>
#include <iostream>
#include <vector>

#include "game_advertiser.hxx"
#include "antenna.hxx"
#include "tuner.hxx"
#include "abuhops.hxx"
#include "io.hxx"

using namespace std;

static const byte trigger[] = {'A', 'b', 'e', 'n', 'd', 's', 'u', 'c', 'h'};

GameAdvertiser::GameAdvertiser(Tuner* tuner_,
                               bool v6_,
                               Uint32 overseerid_,
                               byte peerCount_,
                               bool passwordProtected_,
                               const char* gameMode_,
                               bool isInternet_)
: tuner(tuner_), v6(v6_),
  overseerid(overseerid_),
  peerCount(peerCount_),
  passwordProtected(passwordProtected_),
  isInternet(isInternet_)
{
  setGameMode(gameMode_);
  if (!isInternet)
    tuner->trigger(trigger, sizeof(trigger), this);
}

GameAdvertiser::~GameAdvertiser() {
  if (!isInternet)
    tuner->untrigger(trigger, sizeof(trigger));
}

void GameAdvertiser::setOverseerId(Uint32 oid) {
  overseerid = oid;
  postIfNeeded();
}

void GameAdvertiser::setPeerCount(byte cnt) {
  peerCount = cnt;
  postIfNeeded();
}

void GameAdvertiser::setGameMode(const char* mode) {
  strncpy(gameMode, mode, 4);
  postIfNeeded();
}

void GameAdvertiser::process(const Antenna::endpoint& source,
                             Antenna* antenna, Tuner* tuner,
                             const byte* data, unsigned len)
noth {
  //Ignore if it is the wrong protocol, or this is an Internet game
  if (v6 == (source.protocol() == asio::ip::udp::v6()) && !isInternet) {
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

void GameAdvertiser::postIfNeeded() {
  if (isInternet) {
    vector<byte> pack(10 + 4 + 2 + 2*(v6? 8*2+2 : 4+2) + 4);
    memcpy(&pack[0], "Abendspiel", 10);
    byte* dst = &pack[10];
    io::write(dst, overseerid);
    io::write(dst, peerCount);
    io::write(dst, (byte)v6);
    if (v6) {
      GlobalID& gid(*antenna.getGlobalID6());
      io::a6fromhbo(dst, gid.la6);
      dst += 2*8;
      io::write(dst, gid.lport);
      io::a6fromhbo(dst, gid.ia6);
      dst += 2*8;
      io::write(dst, gid.iport);
    } else {
      GlobalID& gid(*antenna.getGlobalID4());
      memcpy(dst, gid.la4, 4);
      dst += 4;
      io::write(dst, gid.lport);
      memcpy(dst, gid.ia4, 4);
      dst += 4;
      io::write(dst, gid.iport);
    }

    memcpy(dst, gameMode, sizeof(gameMode));

    abuhops::post(v6, &pack[0], pack.size());
  }
}
