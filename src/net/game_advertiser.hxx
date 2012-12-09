/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.06
 * @brief Contains the GameAdvertiser packet processing class
 */
#ifndef GAME_ADVERTISER_HXX_
#define GAME_ADVERTISER_HXX_

#include <SDL.h>

#include "packet_processor.hxx"

/**
 * Listens for "Abendsuch" packets from unknown hosts and replies with
 * Abendstern game information.
 *
 * The class must be configured for EITHER IPv4 or IPv6, since all games
 * must use the same protocol for all peers.
 *
 * The destructor automatically untriggers the GameAdvertiser.
 */
class GameAdvertiser: public PacketProcessor {
  Tuner*const tuner;
  bool v6;
  Uint32 overseerid;
  byte peerCount, passwordProtected;
  bool isInternet;
  char gameMode[4];

public:
  /**
   * Creates and registers the GameAdvertiser.
   * @param tuner the Tuner to register with
   * @param v6 if true, use IPv6; otherwise, use IPv4
   * @param overseerid the current peer ID of the overseeh
   * @param peerCount the current number of peers in the game
   * @param passwordProtected whether a password is required to join the game
   * @param gameMode a string describing the game mode; the first four
   * characters are used if it is longer than 4.
   * @param isInternet Whether this is an Internet (via Abuhops) or LAN game.
   */
  GameAdvertiser(Tuner* tuner, bool v6,
                 Uint32 overseerid, byte peerCount,
                 bool passwordProtected, const char* gameMode,
                 bool isInternet);
  virtual ~GameAdvertiser();

  /**
   * Alters the overseer peer ID to the given value.
   */
  void setOverseerId(Uint32);
  /**
   * Alters the peer count to the given value.
   */
  void setPeerCount(byte);
  /**
   * Alters the game mode to the given string. Only the first four characters
   * are used.
   */
  void setGameMode(const char*);

  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth;

private:
  void postIfNeeded();
};

#endif /* GAME_ADVERTISER_HXX_ */
