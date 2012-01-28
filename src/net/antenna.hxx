/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Antenna network abstraction class.
 */

/*
 * antenna.hxx
 *
 *  Created on: 09.12.2011
 *      Author: jason
 */

#ifndef ANTENNA_HXX_
#define ANTENNA_HXX_

#include <asio.hpp>

#include "globalid.hxx"
#include "src/core/aobject.hxx"

class Tuner;

/**
 * The Antenna class abstracts away some of the immediate interface
 * for sending network packets. It also handles concerns like port
 * and IP version selection, as well as determining the local peer's
 * global-unique id.
 *
 * An Antenna is associated with an IPv4 and an IPv6 socket. When
 * created, it will attempt to open each of them on one of the
 * "well-known" Abendstern ports (see Antenna::wellKnownPorts);
 * if none are available, it falls back on an ephemeral port. It
 * will attempt to connect each of them to abendstern.servegame.com
 * if appropriate DNS entries can be found; otherwise, it falls back
 * on 192.0.43.10 for IPv4 and ::FFFF::C000:2B0A (example.com) as a
 * generic Internet address. (The "connection" is done solely so that
 * we can get the local address of the port to use.) The sockets
 * always have broadcasting enabled.
 *
 * While primarily used as a singleton, there are no conflicts in
 * using multiple instances of this class.
 */
class Antenna: public AObject {
  //The sockets the Antenna operates on
  asio::ip::udp::socket sock4, sock6;
  //The globalids
  GlobalID gid4, gid6;

public:
  ///The Asio endpoint type used
  typedef asio::ip::udp::endpoint endpoint;

  ///Default constructor
  Antenna();
  virtual ~Antenna();

  /**
   * The Tuner to use to examine incomming packets.
   * If this is non-NULL, all incomming packets will be passed through
   * the Tuner's receivePacket() function.
   * The pointee is owned by the Antenna; it will be deleted when the
   * Antenna is destructed.
   */
  Tuner* tuner;

  /**
   * These are the "well-known" port numbers (well-known to Abendstern,
   * not 0..1023) that most Abendstern peers are expected to use.
   * The well-known ports are necessary for peer discovery for LAN games;
   * automatic detection broadcasts to these ports only (since searching
   * all 65536 would take too long and would have little to no benefit).
   */
  static const unsigned short wellKnownPorts[4];

  /**
   * Sets the IPv4 Internet-facing information in the GlobalID with the
   * given information, assumed to have been received from the Abendstern
   * Network server.
   *
   * This function is intended to be called by Tcl, so it takes the address
   * as separate arguments.
   */
  void setInternetInformation4(unsigned char iaddr0,
                               unsigned char iaddr1,
                               unsigned char iaddr2,
                               unsigned char iaddr3,
                               unsigned short iport) noth;
  /**
   * Sets the IPv6 Internet-facing information in the GlobalID with
   * the given information, assumed to have been received from the
   * Abendstern Network server.
   *
   * This function is intended to be called by Tcl, so it takes the address
   * as separate arguments.
   */
  void setInternetInformation6(unsigned short iaddr0,
                               unsigned short iaddr1,
                               unsigned short iaddr2,
                               unsigned short iaddr3,
                               unsigned short iaddr4,
                               unsigned short iaddr5,
                               unsigned short iaddr6,
                               unsigned short iaddr7,
                               unsigned short iport) noth;

  /**
   * Returns a pointer to the IPv4 GlobalID associated with this Antenna.
   */
  GlobalID* getGlobalID4() noth;
  /**
   * Returns a pointer to the IPv4 GlobalID associated with this Antenna.
   */
  const GlobalID* getGlobalID4() const noth;
  /**
   * Returns a pointer to the IPv6 GlobalID associated with this Antenna.
   */
  GlobalID* getGlobalID6() noth;
  /**
   * Returns a pointer to the IPv6 GlobalID associated with this Antenna.
   */
  const GlobalID* getGlobalID6() const noth;

  /**
   * Sends the given data to the given endpoint.
   * The input array is not deallocated.
   */
  void send(const endpoint&, const byte* data, unsigned length) noth;

  /**
   * Processes any incomming packets.
   */
  void processIncomming() noth;
};


#endif /* ANTENNA_HXX_ */
