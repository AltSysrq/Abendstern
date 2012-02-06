/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.06
 * @brief Contains the GameDiscoverer packet processor
 */
#ifndef GAME_DISCOVERER_HXX_
#define GAME_DISCOVERER_HXX_

#include <vector>

#include <asio.hpp>
#include <SDL.h>

#include "packet_processor.hxx"

/**
 * Provides the ability to discover advertised LAN games within broadcast
 * range.
 */
class GameDiscoverer: public PacketProcessor {
  struct Result {
    asio::ip::udp::endpoint peer;
    Uint32 overseer;
    bool passwordProtected;
    byte peerCount;
    char gameMode[4];
  };
  std::vector<Result> results;

  Uint32 nextBroadcast;
  unsigned currentTry;

public:
  /**
   * Creates an empty GameDiscoverer and registers it with the given Tuner.
   */
  GameDiscoverer(Tuner*);
  /**
   * Clears the results list and resets internal data, making it ready for
   * a new scan.
   */
  void start() noth;
  /**
   * Updates internal state and may broadcast requests. Calling this is required
   * to get results.
   */
  void poll(Antenna*) noth;
  /**
   * Returns the current progress of the entire scan, between 0 and 1,
   * inclusive.
   */
  float progress() const noth;

  /**
   * Dumps the results to stdout.
   * This function is for debugging; as of 2012.02.06, there are currently no
   * functions to access the actual results data.
   */
  void dumpResults() const noth;

  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth;
};

#endif /* GAME_DISCOVERER_HXX_ */
