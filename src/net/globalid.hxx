/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GlobalID class
 */

/*
 * globalid.hxx
 *
 *  Created on: 09.12.2011
 *      Author: jason
 */

#ifndef GLOBALID_HXX_
#define GLOBALID_HXX_

#include <string>

#include "src/core/aobject.hxx"

/**
 * The GlobalID class defines a structure that globally and uniquely
 * identifies a peer (for the short term). It also allows determining
 * how to connect to such peers.
 * It should not be confused with a propper GUID.
 */
class GlobalID: public AObject {
  public:
  ///Tag for the union below; the IP version in use
  enum IPVersion {
    IPv4, ///<Internet Protocol version 4
    IPv6  ///<Internet Protocol version 6
  } ipv; ///< Determines which struct in the union is in use
  union {
    ///IPv4 ID, only valid if ipv==IPv4
    struct {
      ///Internet-facing address
      unsigned char ia4[4];
      ///Lan-facing address
      unsigned char la4[4];
    };
    ///IPv6 ID, only valid if ipv==IPv6
    struct {
      ///Internet-facing address
      unsigned short ia6[8];
      ///LAN-facing address
      unsigned short la6[8];
    };
  };
  ///Internet-facing port number
  unsigned short iport;
  ///LAN-facing port number
  unsigned short lport;

  /**
   * Returns the string representation of the ID.
   * For a v4 ID, the format is as follows:
   *   %03d.%03d.%03d.%03d:%05d/%03d.%03d.%03d.%03d:%05d
   * and for v6
   *   %04x.%04x.%04x.%04x.%04x.%04x.%04x.%04x:%05d/%04x.%04x.%04x.%04x.%04x.%04x.%04x.%04x:%05d
   * Note that the v6 format uses dots in the IPv6 address, since colons are
   * used for ports.
   *
   * In both cases, the overall format is
   *   lan-address:lan-port/internet-address:internet-port
   */
  std::string toString() const;
};

#endif /* GLOBALID_HXX_ */
