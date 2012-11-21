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
#include <cstring>

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
    IPv4=0, ///<Internet Protocol version 4
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

  GlobalID() {
    //Set the data portion of the GlobalID to all zeros.
    std::memset(&ipv, 0,
                sizeof(GlobalID) -
                //Poor man's offsetof (since we aren't allowed to use that on
                //non-POD classes)
                (reinterpret_cast<const char*>(&ipv) -
                 reinterpret_cast<const char*>(this)));
  }

  //Copy constructor which copies the entire data section (including padding)
  //Without this, the memcmp-based comparison has issues with uninitialised
  //data in the padding between data
  GlobalID(const GlobalID& that) {
    std::memcpy(&ipv, &that.ipv,
                sizeof(GlobalID) -
                //Poor man's offsetof (since we aren't allowed to use that on
                //non-POD classes)
                (reinterpret_cast<const char*>(&ipv) -
                 reinterpret_cast<const char*>(this)));
  }

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

  //What happened to the default operator== ?
  bool operator==(const GlobalID& that) const {
    return
      this->ipv == that.ipv &&
      this->iport == that.iport &&
      this->lport == that.lport &&
      (ipv != IPv4 || !memcmp(this->ia4, that.ia4, sizeof(ia4))) &&
      (ipv != IPv4 || !memcmp(this->la4, that.la4, sizeof(la4))) &&
      (ipv != IPv6 || !memcmp(this->ia6, that.ia6, sizeof(ia6))) &&
      (ipv != IPv6 || !memcmp(this->la6, that.la6, sizeof(la6)));
  }
};

#endif /* GLOBALID_HXX_ */
