/**
 * @file
 * @author Jason Linge
 * @date 2012.04.18
 * @brief Implementation of the WindowsFirewallOpener
 */

#include <asio.hpp>

#include "windows_firewall_opener.hxx"
#include "antenna.hxx"
#include "src/globals.hxx"

#define INTERVAL 4096

WindowsFirewallOpener::WindowsFirewallOpener(Antenna* a)
: antenna(a), timeSinceTransmission(INTERVAL)
{
}

void WindowsFirewallOpener::update(unsigned et) throw() {
  static const asio::ip::address v4addr(
    asio::ip::address_v4::from_string("255.255.255.255"));
  static const asio::ip::address v6addr(
    asio::ip::address_v6::from_string("ff02::1"));

  timeSinceTransmission += et;
  if (timeSinceTransmission > INTERVAL) {
    //Send broadcasts on both protocols and on all well-known ports
    for (unsigned port = 0; port < lenof(Antenna::wellKnownPorts); ++port) {
      if (antenna->hasV6()) {
        asio::ip::udp::endpoint dest(v6addr, Antenna::wellKnownPorts[port]);
        try {
          antenna->send(dest, NULL, 0);
        } catch (...) {}
      }
      if (antenna->hasV4()) {
        asio::ip::udp::endpoint dest(v4addr, Antenna::wellKnownPorts[port]);
        try {
          antenna->send(dest, NULL, 0);
        } catch (...) {}
      }
    }
  }
}
