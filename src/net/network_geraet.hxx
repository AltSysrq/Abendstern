/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkGeraet class
 */
#ifndef NETWORK_GERAET_HXX_
#define NETWORK_GERAET_HXX_

#include "antenna.hxx"
#include "network_connection.hxx"

/**
 * Defines the interface common to all input network Geräte.
 */
class InputNetworkGeraet: public AObject {
public:
  /**
   * Notifies the InputNetworkGeraet that it has received a packet.
   * @param seq the sequence number of the packet
   * @param data the payload of the packet, excluding sequence and channel
   * numbers
   * @param len the length of the payload
   */
  virtual void receive(unsigned short seq, const byte* data, unsigned len)
  throw() = 0;

  /**
   * Updates the InputNetworkGeraet based on elapsed time.
   *
   * The default does nothing.
   * @param et the time, in milliseconds, since the previous call to update()
   */
  virtual void update(unsigned et) throw() { }
};

/**
 * Defines the interface common to all output network Geräte.
 */
class OutputNetworkGeraet: public AObject {
protected:
  /**
   * The output channel to write to.
   */
  NetworkConnection::channel channel;
public:
  /**
   * Updates the OutputNetworkGeraet based on elapsed time.
   *
   * The default does nothing.
   * @param et the time, in milliseconds, since the previous call to update()
   */
  virtual void update(unsigned et) throw() { }
};
#endif /* NETWORK_GERAET_HXX_ */
