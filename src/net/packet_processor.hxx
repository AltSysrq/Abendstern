/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.28
 * @brief Contains the PacketProcessor network class
 */
#ifndef PACKET_PROCESSOR_HXX_
#define PACKET_PROCESSOR_HXX_

#include "src/core/aobject.hxx"
#include "antenna.hxx"
class Tuner;

/**
 * Abstract class to process incomming packets.
 */
class PacketProcessor: public AObject {
public:
  /**
   * Processes a single incomming packet.
   * @param source the source of this packet
   * @param antenna the Antenna that is receiving/sending the data
   * @param tuner the Tuner forwarding the data to the packet processor
   * @param data the payload of the packet. The memory backing this
   * argument is managed by the caller, and no assumptions about it
   * or its contents may be made after this function returns; if the
   * data is needed later, it must be copied.
   * @param len the length of the data
   */
  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth = 0;
};

#endif /* PACKET_PROCESSOR_HXX_ */
