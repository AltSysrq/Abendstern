/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.28
 * @brief Contains the PacketProcessor network class
 */
#ifndef PACKET_PROCESSOR_HXX_
#define PACKET_PROCESSOR_HXX_

#include "src/aobject.hxx"
#include "antenna.hxx"

/**
 * Abstract class to process incomming packets.
 */
class PacketProcessor: public AObject {
public:
  /**
   * Processes a single incomming packet.
   * @param source the source of this packet
   * @param data the payload of the packet
   * @param len the length of the data
   */
  virtual void process(const Antenna::endpoint& source,
                       const byte* data, unsigned len) noth = 0;
};

#endif /* PACKET_PROCESSOR_HXX_ */
