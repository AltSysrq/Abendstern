/**
 * @file
 * @author Jason Lingle
 * @date 2012.01.28
 * @brief Contains the Tuner networking class
 */

#ifndef TUNER_HXX_
#define TUNER_HXX_

#include <map>
#include <vector>
#include <list>

#include "src/core/aobject.hxx"
#include "antenna.hxx"

class PacketProcessor;

/**
 * Uses the source and/or headers of packets to select PacketProcessors.
 *
 * The Tuner class examines raw packets and their sources to determine an
 * appropriate PacketProcessor to use for the packet.
 *
 * Whenever a packet is passed through the Tuner::packetReceived() function,
 * it first checks to see whether a PacketProcessor is associated with the
 * source endpoint; if it is, that PacketProcessor is used. Otherwise, a
 * list of headers is searched, from longest to shorstest, until the first
 * bytes of the packet match a header. When such a match is found, that
 * PacketProcessor is used; otherwise, the packet is silently discarded.
 */
class Tuner: public AObject {
  //Map of endpoints to PacketProcessors, for source-based demuxing
  std::map<Antenna::endpoint, PacketProcessor*> connections;
  //Header->PacketProcessor mapping
  std::list<std::pair<std::vector<byte>, PacketProcessor*> > headers;

public:
  /**
   * Creates an empty Tuner instance.
   */
  Tuner();

  /**
   * Associates the given endpoint with the given PacketProcessor.
   * If such an association already exists, it is silently overwritten.
   * The PacketProcessor will not be freed by the Tuner.
   */
  void connect(const Antenna::endpoint&, PacketProcessor*) noth;
  /**
   * Disassociates the given endpoint from its PacketProcessor.
   * The association MUST exist.
   * The PacketProcessor is not freed.
   * @return the PacketProcessor that was bound to the endpoint
   */
  PacketProcessor* disconnect(const Antenna::endpoint&) noth;

  /**
   * Associates the given packet header with the given PacketProcessor.
   * If such an association already exists, it is silently overwritten.
   * @param data the header data; not freed by this call
   * @param len the length of the header
   * @param pp the PacketProcessor to associate; will not be freed by the Tuner
   */
  void trigger(const byte* data, unsigned len, PacketProcessor* pp) noth;
  /**
   * Disassociates the given packet header from its PacketProcessor.
   * The association MUST exist.
   * The PacketProcessor is not freed.
   * @param data the header data; not freed by this call
   * @param len the length of the header
   * @return the PacketProcessor that was bound to the header
   */
  PacketProcessor* untrigger(const byte* data, unsigned len) noth;

  /**
   * Examines the given packet and its source, and may process it if
   * any PacketProcessor triggers for it.
   */
  void receivePacket(const Antenna::endpoint& source,
                     const byte* data, unsigned len) const noth;
};

#endif /* TUNER_HXX_ */
