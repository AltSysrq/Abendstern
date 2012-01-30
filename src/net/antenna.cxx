/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.28
 * @brief Implementation of the Antenna class
 */

#include <cstdlib>

#include <asio.hpp>

#include "antenna.hxx"
#include "tuner.hxx"
#include "src/globals.hxx"

using namespace std;

static asio::io_service iosvc;
Antenna antenna;

const unsigned short Antenna::wellKnownPorts[4] = {
//ABEND, BENDS, ENDST, NDSTE
  12544, 25449, 54490, 44905
};

Antenna::Antenna() : sock4(NULL), sock6(NULL) {
  gid4.ipv = GlobalID::IPv4;
  gid6.ipv = GlobalID::IPv6;

  /**
   * Our primary UDP sockets will be open on INADDR_ANY, so that we can
   * receive packets from anywhere. However, such ports will return
   * INADDR_ANY for their local endpoint, so we need to create a socket
   * to some Internet endpoint (not necessarily a valid one) in order
   * to get this information.
   */

  //Try to open an IPv4 socket to ANY
  for (unsigned i=0; i<lenof(wellKnownPorts) && !sock4; ++i) {
    try {
      sock4 = new asio::ip::udp::socket(iosvc,
                                        endpoint(asio::ip::address(
                                                   asio::ip::address_v4::any()),
                                                 wellKnownPorts[i]));
      gid4.lport = wellKnownPorts[i];
    } catch (...) {}
  }

  if (sock4) {
    //Create an Internet-facing socket to get the local address
    try {
      asio::ip::udp::socket tmpsock(iosvc, asio::ip::udp::v4());
      tmpsock.connect(endpoint(
                        asio::ip::address(asio::ip::address_v4(0xC0002B0A)),
                        12544));
      const asio::ip::address_v4 lip4 =
          tmpsock.local_endpoint().address().to_v4();

      for (unsigned i=0; i<4; ++i)
        gid4.la4[i] = lip4.to_bytes()[i];

      cout << "Our local IPv4 address/port is: "
           << tmpsock.local_endpoint().address()
           << ":" << gid4.lport << endl;
    } catch (asio::system_error& err) {
      cerr << "Could not determine address of local IPv4 endpoint: "
           << err.what() << endl;
      delete sock4;
      sock4 = NULL;
    }
  } else {
    cerr << "Could not open IPv4 socket, IPv4 networking will be unavailable."
         << endl;
  }

  //Now for IPv6
  for (unsigned i=0; i<lenof(wellKnownPorts) && !sock6; ++i) {
    try {
      sock6 = new asio::ip::udp::socket(iosvc,
                                        endpoint(asio::ip::address(
                                                   asio::ip::address_v6::any()),
                                                 wellKnownPorts[i]));
      gid6.lport = wellKnownPorts[i];
    } catch (...) {}
  }

  if (sock6) {
    //Create Internet-facing socket for the local address
    try {
      asio::ip::udp::socket tmpsock(iosvc, asio::ip::udp::v6());
      tmpsock.connect(endpoint(
                        asio::ip::address(asio::ip::address_v6::from_string(
                                            "::FFFF:C000:2B0A")),
                        12544));
      const asio::ip::address_v6::bytes_type lip6 =
          tmpsock.local_endpoint().address().to_v6().to_bytes();

      for (unsigned i=0; i<8; ++i) {
        unsigned short msb = lip6[i*2];
        unsigned short lsb = lip6[i*2+1];
        gid6.la6[i] = (msb << 8) | lsb;
      }
      cout << "Our local IPv6 address/port is: "
           << tmpsock.local_endpoint().address()
           << " : " << gid6.lport << endl;
    } catch (asio::system_error& err) {
      cerr << "Could not determine address of local IPv6 endpoint: "
           << err.what() << endl;
      delete sock6;
      sock6 = NULL;
    }
  } else {
    cerr << "Could not open IPv6 socket, IPv6 networking will be unavailable."
         << endl;
  }
}

Antenna::~Antenna() {
  if (sock4) delete sock4;
  if (sock6) delete sock6;
}
