/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.28
 * @brief Implementation of the Antenna class
 */

#include <cstdlib>
#include <iostream>
#include <cassert>

#include <asio.hpp>

#include "antenna.hxx"
#include "tuner.hxx"
#include "src/globals.hxx"

using namespace std;

static asio::io_service iosvc;
Antenna antenna;
unsigned packetDropMask = 0;

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
  asio::ip::udp::socket::broadcast enableBroadcast(true);

  //Try to open an IPv4 socket to ANY
  for (unsigned i=0; i<lenof(wellKnownPorts) && !sock4; ++i) {
    try {
      sock4 = new asio::ip::udp::socket(iosvc,
                                        endpoint(asio::ip::address(
                                                   asio::ip::address_v4::any()),
                                                 wellKnownPorts[i]));
      sock4->set_option(enableBroadcast);
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
      sock6->set_option(enableBroadcast);
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

void Antenna::setInternetInformation4(unsigned char a0,
                                      unsigned char a1,
                                      unsigned char a2,
                                      unsigned char a3,
                                      unsigned short port) noth {
  gid4.ia4[0] = a0;
  gid4.ia4[1] = a1;
  gid4.ia4[2] = a2;
  gid4.ia4[3] = a3;
  gid4.iport = port;
}

void Antenna::setInternetInformation6(unsigned short a0,
                                      unsigned short a1,
                                      unsigned short a2,
                                      unsigned short a3,
                                      unsigned short a4,
                                      unsigned short a5,
                                      unsigned short a6,
                                      unsigned short a7,
                                      unsigned short port) noth {
  gid6.ia6[0] = a0;
  gid6.ia6[1] = a1;
  gid6.ia6[2] = a2;
  gid6.ia6[3] = a3;
  gid6.ia6[4] = a4;
  gid6.ia6[5] = a5;
  gid6.ia6[6] = a6;
  gid6.ia6[7] = a7;
  gid6.iport = port;
}

void Antenna::send(const endpoint& dst, const byte* data, unsigned len)
throw (asio::system_error) {
  if (dst.protocol() == asio::ip::udp::v4()) {
    assert(sock4);
    sock4->send_to(asio::buffer(data, len), dst);
  } else {
    assert(dst.protocol() == asio::ip::udp::v6());
    assert(sock6);
    sock6->send_to(asio::buffer(data, len), dst);
  }
}

void Antenna::processIncomming() throw (asio::system_error) {
  static byte data[65536];
  if (sock4) while (sock4->available()) {
    endpoint source;
    unsigned len = sock4->receive_from(asio::buffer(data, sizeof(data)),
                                       source);
    if (rand() & packetDropMask) continue;
    if (len && tuner)
      tuner->receivePacket(source, this, data, len);
  }

  if (sock6) while (sock6->available()) {
    endpoint source;
    unsigned len = sock6->receive_from(asio::buffer(data, sizeof(data)),
                                       source);

    if (rand() & packetDropMask) continue;
    if (len && tuner)
      tuner->receivePacket(source, this, data, len);
  }
}

void Antenna::close() noth {
  if (sock4) {
    try {
      sock4->shutdown(asio::ip::udp::socket::shutdown_both);
    } catch (...) {}
    try {
      sock4->close();
    } catch (...) {}
  }
  if (sock6) {
    try {
      sock6->shutdown(asio::ip::udp::socket::shutdown_both);
    } catch (...) {}
    try {
      sock6->close();
    } catch (...) {}
  }
}
