#ifndef WINDOWS_FIREWALL_OPENER_HXX_
#define WINDOWS_FIREWALL_OPENER_HXX_

/**
 * @file
 * @author Jason Lingle
 * @date 2012.04.18
 * @brief Provides the WindowsFirewallOpener class
 */

class Antenna;

/**
 * Sends packets to open the Windows firewall.
 *
 * Normally, Abendstern would require a Windows firewall exception to work on
 * LAN games, since it requires receiving a unicast packet from an unknown
 * source. However, non-administrators do not have the privelages needed to add
 * an exception.
 *
 * But, according to the Windows Firewall Technical Reference, "When a
 * computer sends a multicast or broadcast message, Windows Firewall waits for
 * up to 3 seconds for a unicast response."
 *
 * This class will send an empty broadcast packet once four seconds, to keep
 * this rule active. This means that no firewall exception is necessary. (The
 * four second interval allows Windows to forget the "connection" and start a
 * new one.)
 *
 * @see http://technet.microsoft.com/en-us/library/cc755604(v=ws.10).aspx#BKMK_protocols
 */
class WindowsFirewallOpener {
  Antenna*const antenna;
  unsigned timeSinceTransmission;

public:
  ///Creates the WindowsFirewallOpener for the given Antenna
  WindowsFirewallOpener(Antenna*);

  ///Updates the timer and may send a packet.
  void update(unsigned) throw();
};

#endif /* WINDOWS_FIREWALL_OPENER_HXX_ */
