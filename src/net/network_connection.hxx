/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkConnection class
 */
#ifndef NETWORK_CONNECTION_HXX_
#define NETWORK_CONNECTION_HXX_

#include <map>
#include <deque>
#include <set>
#include <string>
#include <bitset>
#include <queue>

#include "src/sim/game_field.hxx"
#include "packet_processor.hxx"
#include "antenna.hxx"

class InputNetworkGeraet;
class OutputNetworkGeraet;
class NetworkAssembly;
class SynchronousControlGeraet;
class LatDiscGeraet;
class AsyncAckGeraet;

/**
 * Encapsulates all information pertaining to a connection to another peer.
 *
 * It specifically maintains the following information:
 * <ul>
 *   <li>A map of open channels to their NetworkGeraete</li>
 *   <li>A map of unique Geraet numbers to active NetworkGeraete</li>
 *   <li>A separate GameField that mirrors what the remote peer sees, allowing
 *       for more intelligent decisions regarding packet dispatch.</li>
 *   <li>A map of exported GameObjects to those in said mirror</li>
 *   <li>The average latency (maintained by the LatencyDiscoveryGeraet).</li>
 *   <li>The last time of receiving a packet, to disconnect if the connection
 *       drops.</li>
 *   <li>A set of recently received sequence numbers</li>
 *   <li>A queue of numbers to remove from above set</li>
 *   <li>The greatest recently received sequence number</li>
 * </ul>
 */
class NetworkConnection: public PacketProcessor {
  friend class SynchronousControlGeraet;
  friend class LatDiscGeraet;
  friend class NetworkAssembly;

public:
  ///Type to use to identify channels
  typedef unsigned short channel;
  ///Type to use to identify Gerät types
  typedef unsigned short geraet_num;
  ///Type for sequence numbers
  typedef unsigned short seq_t;
  ///Function pointer to construct InputNetworkGeraete by number
  typedef InputNetworkGeraet* (*geraet_creator)(NetworkConnection*);

  /**
   * Defines the possible stati of a NetworkConnection.
   */
  enum Status {
    Connecting, ///< Establishing outgoing connection, have not heard back yet
    Established, ///< Two-way communication established, not yet set up
    Ready, ///< Fully functional
    Zombie ///< Connection closed
  };

  /**
   * The remote mirror field to use to keep track of objects exported
   * to this peer.
   */
  GameField field;

private:
  //Local and remote channel mapping
  typedef std::map<channel,InputNetworkGeraet*> inchannels_t;
  inchannels_t inchannels;
  typedef std::map<channel,OutputNetworkGeraet*> outchannels_t;
  outchannels_t outchannels;
  //Next sequence numbers
  seq_t nextOutSeq;

  //Map Gerät numbers to their creators
  typedef std::map<geraet_num, geraet_creator> geraetNumMap_t;
  static geraetNumMap_t* geraetNumMap;

  //Set of up to 1024 recently received seqs, for duplicate removal
  std::set<seq_t> recentlyReceived;
  //Queue of recently received seqs, in order, to properly remove from
  //the above set.
  std::deque<seq_t> recentlyReceivedQueue;
  /* The greatest recently received seq. Any new packets will be dropped if
   * the following equation does not hold (taking integer wrapping into
   * account):  (newSeq-greatestSeq < 1024 || greatestSeq-newSeq < 1024).
   * This is updated to the new seq if: (newSeq-greatestSeq < 1024).
   */
  seq_t greatestSeq;

  //Average latency, in milliseconds
  unsigned latency;
  //Last time (via SDL_GetTicks()) that we received a packet
  unsigned lastIncommingTime;

  Status status;

  std::string disconnectReason;

  //Sequence numbers which are marked as "locked"
  //(see lock() and release())
  std::bitset<65536> locked;

  //Contains GameObject*s that are candidates for exportation.
  //That is, they are exportable, but have not yet been exported,
  //either because they were just created, or because
  //they are transient and are too far away to matter (and were
  //recently moved back from ignoredExports).
  std::deque<GameObject*> candidateExports;
  //Contains GameObject*s that have been exported to the remote peer.
  std::set<GameObject*> actualExports;
  //Contains transient, exportable GameObject*s that were not exported
  //due to distance from all of the remote peer's references.
  //This periodically emptied back into candidateExports.
  std::set<GameObject*> ignoredExports;

  //Contains remote GameObject*s that are used as a reference point
  //for determining whether to export transient objects.
  std::vector<GameObject*> remoteReferences;

  //The time since we last emptied ignoredExports into candidateExports.
  unsigned timeSinceRetriedTransients;

  //Queue of channel numbers to close once safe
  std::queue<channel> channelsToClose;

  //Networking statistics
  Uint32 lastStatsUpdate, connectionStart;
  unsigned packetInCount, packetInCountSec, packetInCountSecMax;
  unsigned packetOutCount, packetOutCountSec, packetOutCountSecMax;
  unsigned long long dataInCount, dataInCountSec, dataInCountSecMax;
  unsigned long long dataOutCount, dataOutCountSec, dataOutCountSecMax;

public:
  ///The endpoint of the remote peer
  const Antenna::endpoint endpoint;
  ///The parent NetworkAssembly
  NetworkAssembly*const parent;
  ///The SCG used with this NetworkConnection
  SynchronousControlGeraet*const scg;
  ///The AAG used with this NetworkConnection
  AsyncAckGeraet*const aag;
  ///The LDG used with this NetworkConnection
  LatDiscGeraet* const ldg;

  /**
   * Constructs a NetworkConnection within the given assembly.
   * @param assembly the parent NetworkAssembly
   * @param endpoint the remote endpoint
   * @param incomming if true, assume we have received a 0-0-0-0-2 message
   * already, so the next incomming seq should be 1 instead of 0, and no
   * STX will be expected.
   */
  NetworkConnection(NetworkAssembly* assembly,
                    const Antenna::endpoint& endpoint,
                    bool incomming);
  virtual ~NetworkConnection();

  /**
   * Performs time-based updates and sends outgoing packets.
   *
   * @param et elapsed time, in milliseconds, since the last call to update()
   */
  void update(unsigned et) noth;

  /**
   * Returns the current status of the connection.
   */
  Status getStatus() const noth { return status; }

  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth;

  /**
   * Registers the given Gerät creator, returning the Gerät number
   * associated with it.
   * @param fun the geraet_creator to register
   * @param num the number to use for the Gerät; if equal to ~0 (the default),
   * a number is chosen automatically. If not ~0, the number must not be in
   * use already. Automatically-allocated numbers start at 32768.
   * @return the Gerät number of the creator
   */
  static geraet_num registerGeraetCreator(geraet_creator fun,
                                          geraet_num num=~0);
  /**
   * Returns the geraet_creator associated with the given Gerät number.
   * The result is NULL if the number is not associated with any
   * Gerät creator.
   */
  static geraet_creator getGeraetCreator(geraet_num);

  /**
   * Returns the sequence number for the next packet, and increments
   * the internal counter.
   */
  seq_t seq() noth;

  /**
   * Writes the packet header to the given packet, advancing the pointer
   * past the packet.
   * @returns the seq of the packet
   */
  seq_t writeHeader(byte*& dst, channel chan) noth;

  /** The size of the common packet header, in bytes. */
  static const unsigned headerSize = 4;

  /**
   * Sends the given data in a packet to the remote peer.
   * This never throws an exception; if something goes wrong,
   * the connection moves to the Zombie status, and sets
   * its disconnectReason appropriately.
   */
  void send(const byte* data, unsigned len) throw();

  /**
   * Returns the reason for disconnection.
   */
  const std::string& getDisconnectReason() const noth {
    return disconnectReason;
  }

  /**
   * Returns the latency (round-trip time) of the connection, in ms.
   */
  unsigned getLatency() const noth {
    return latency;
  }

  /**
   * Marks the given sequence number as "locked".
   *
   * This status indicates that the caller attaches other information to that
   * number, and that network collapse has occurred if it is reused.
   *
   * This does not prevent the reuse of this number --- rather, it will trigger
   * a disconnect on detecting reuse (which subsequently results in a reuse
   * of the packet number anyway). This is merely an error detector.
   *
   * Calls to lock() must be complemented by a later call to release().
   *
   * @see release()
   */
  void lock(seq_t seq) throw() { locked.set(seq); }
  /**
   * Marks the given sequence number as "unlocked".
   *
   * @see lock()
   */
  void release(seq_t seq) throw() { locked.reset(seq); }

  /**
   * Alters the size of the internal game field to the specified
   * dimenions.
   * @see GameField::width
   * @see GameField::height
   */
  void setFieldSize(float w, float h) throw() {
    field.width = w;
    field.height = h;
  }

  /**
   * Marks the given GameObject*, which should be remote comming from this
   * peer, as a reference. References are used to determine the "distance"
   * for purpose of exporting transients and rate of sending updates.
   */
  void setReference(GameObject*) throw();
  /**
   * Makes the given GameObject* as not a reference, if it was one already.
   *
   * @see setReference()
   */
  void unsetReference(GameObject*) throw();

  /**
   * Returns the "distance" of the given GameObject*. This is the minimum
   * distance squared between the object and any reference, or INFINITY if
   * there are no references.
   */
  float distanceOf(const GameObject*) const throw();

  /**
   * Queues the given channel number for closing at a later time, when it
   * is safe to do so.
   * This is typically used from within OutputNetworkGeraet functions which
   * cannot safely directly close the channel at the time.
   */
  void closeChannelWhenSafe(channel chan) throw() {
    channelsToClose.push(chan);
  }

private:
  //Called by NetworkAssembly when an exportable object is added
  void objectAdded(GameObject*) throw();
  //Called by NetworkAssembly when ANY object is removed
  void objectRemoved(GameObject*) throw();
};

#endif /* NETWORK_CONNECTION_HXX_ */
