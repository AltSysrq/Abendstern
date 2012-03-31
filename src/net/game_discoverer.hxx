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
 *
 * The destructor automatically deregisters the GameDiscoverer.
 */
class GameDiscoverer: public PacketProcessor {
public:
  /**
   * Defines a single, unique game discovered on the network.
   */
  struct Result {
    ///An arbitrary peer that can be used to connect to the game
    asio::ip::udp::endpoint peer;
    ///Numeric ID of the overseer
    Uint32 overseer;
    ///Whether a password is required to access the game
    bool passwordProtected;
    ///The number of peers connected to the game
    byte peerCount;
    ///The current game mode; may not be NUL terminated
    char gameMode[4];
  };

private:
  Tuner*const tuner;
  
  std::vector<Result> results;

  Uint32 nextBroadcast;
  unsigned currentTry;

public:
  /**
   * Creates an empty GameDiscoverer and registers it with the given Tuner.
   */
  GameDiscoverer(Tuner*);

  virtual ~GameDiscoverer();

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

  /** Returns the results of the scan, sorted descending by peerCount. */
  const std::vector<Result>& getResults() const noth { return results; }

  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth;
};

#endif /* GAME_DISCOVERER_HXX_ */
