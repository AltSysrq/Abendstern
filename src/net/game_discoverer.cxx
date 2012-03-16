/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.06
 * @brief Implementation of the GameDiscoverer class
 */

#include <cstring>
#include <iostream>

#include <SDL.h>

#include "game_discoverer.hxx"
#include "antenna.hxx"
#include "tuner.hxx"
#include "io.hxx"
#include "src/globals.hxx"

using namespace std;

#define BROADCAST_INTERVAL 128
#define CYCLE_COUNT 8
static const byte trigger[] = {
  'A', 'b', 'e', 'n', 'd', 's', 'p', 'i', 'e', 'l',
};

GameDiscoverer::GameDiscoverer(Tuner* tuner)
: nextBroadcast(-1 /* max value */), curentTry(CYCLE_COUNT)
{
  tuner->trigger(trigger, sizeof(trigger), this);
}

void GameDiscoverer::start() noth {
  nextBroadcast = SDL_GetTicks();
  currentTry = 0;
  results.clear();
}

void GameDiscoverer::poll(Antenna* antenna) noth {
  if (SDL_GetTicks() >= nextBroadcast && currentTry < CYCLE_COUNT) {
    ++currentTry;
    static const byte pack[] = {
      'A', 'b', 'e', 'n', 'd', 's', 'u', 'c', 'h'
    };
    static const asio::ip::address v4addr(
      asio::ip::address_v4::from_string("255.255.255.255"));
    static const asio::ip::address v6addr(
      asio::ip::address_v6::from_string("ff02::1"));
    for (unsigned isV6=0; isV6 < 2; ++isV6) {
      for (unsigned port=0; port < lenof(Antenna::wellKnownPorts); ++port) {
        try {
          if (isV6) {
            //IPv6
            if (antenna->hasV6()) {
              asio::ip::udp::endpoint dest(v6addr,
                                           Antenna::wellKnownPorts[port]);
              antenna->send(dest, pack, sizeof(pack));
            }
          } else {
            if (antenna->hasV4()) {
              asio::ip::udp::endpoint dest(v4addr,
                                           Antenna::wellKnownPorts[port]);
              antenna->send(dest, pack, sizeof(pack));
            }
          }
        } catch (asio::system_error e) {
          cerr << "Unable to broadcast to port "
               << Antenna::wellKnownPorts[port]
               << " with protocol " << (isV6? "IPv6" : "IPv4")
               << ": " << e.what() << endl;
        }
      }
    }

    nextBroadcast = SDL_GetTicks() + BROADCAST_INTERVAL;
  }
}

float GameDiscoverer::progress() const noth {
  if (currentTry == 0) return 0;
  if (currentTry == CYCLE_COUNT && SDL_GetTicks() >= nextBroadcast)
    return 1;

  return (currentTry +
          (SDL_GetTicks()-nextBroadcast)/(float)BROADCAST_INTERVAL)/
         (float)CYCLE_COUNT;
}

void GameDiscoverer::process(const Antenna::endpoint& source,
                             Antenna* antenna, Tuner* tuner,
                             const byte* data, unsigned len)
noth {
  //Past trigger
  data += sizeof(trigger);
  len -= sizeof(trigger);
  if (len != 4+1+1+4) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring Abendspiel of unexpected length: "
         << len << ", from " << source << endl;
    #endif
    return;
  }

  Uint32 oid;
  byte peercnt, pwprot;
  char mode[4];
  io::read(data, oid);
  io::read(data, peercnt);
  io::read(data, pwprot);
  strncpy((char*)mode, (const char*)data, 4);

  //Don't add result if the overseer has the same ID as another
  for (unsigned i=0; i<results.size(); ++i)
    if (results[i].overseer == oid)
      return;

  //OK, add result
  Result res;
  res.peer = source;
  res.overseer = oid;
  res.passwordProtected = pwprot;
  res.peerCount = peercnt;
  memcpy(res.gameMode, mode, sizeof(mode));
  results.push_back(res);
}
