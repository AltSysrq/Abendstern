/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkGeraet class
 */
#ifndef NETWORK_GERAET_HXX_
#define NETWORK_GERAET_HXX_

#include "antenna.hxx"

/**
 * Defines the interface common to all NetworkGeraete.
 */
class NetworkGeraet: public AObject {
public:
  /**
   * Notifies the NetworkGeraet that it has received a packet.
   * @param seq the sequence number of the packet
   * @param data the payload of the packet, excluding sequence and channel
   * numbers
   * @param len the length of the payload
   */
  virtual void receive(unsigned short seq, const byte* data, unsigned len)
  throw() = 0;
};

#endif /* NETWORK_GERAET_HXX_ */
