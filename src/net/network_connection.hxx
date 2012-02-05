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

#include "src/sim/game_field.hxx"
#include "packet_processor.hxx"
#include "antenna.hxx"

class NetworkGeraet;
class NetworkAssembly;
class SynchronousControlGeraet;

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
 *   <li>The average latency (maintained by the LatencyDiscoveryPeer).</li>
 *   <li>The last time of receiving a packet, to disconnect if the connection
 *       drops.</li>
 *   <li>A set of recently received sequence numbers</li>
 *   <li>A queue of numbers to remove from above set</li>
 *   <li>The greatest recently received sequence number</li>
 * </ul>
 */
class NetworkConnection: public PacketProcessor {
  friend class SynchronousControlGeraet;
public:
  ///Type to use to identify channels
  typedef unsigned short channel;
  ///Type to use to identify Gerät types
  typedef unsigned short geraet_num;
  ///Type for sequence numbers
  typedef unsigned short seq_t;
  ///Function pointer to construct NetworkGeraete
  typedef NetworkGeraet* (*geraet_creator)(NetworkConnection*);

  /**
   * Defines the possible stati of a NetworkConnection.
   */
  enum Status {
    Connecting, ///< Establishing outgoing connection, have not heard back yet
    Established, ///< Two-way communication established, not yet set up
    Ready, ///< Fully functional
    Zombie ///< Connection closed
  };

private:
  //Mirror field
  GameField field;

  //Local and remote channel mapping
  typedef std::map<channel,NetworkGeraet*> chanmap_t;
  chanmap_t locchan, remchan;
  //Next sequence numbers
  seq_t nextOutSeq;

  //Unique Gerät number mapping
  typedef std::map<geraet_num,NetworkGeraet*> geraete_t;
  geraete_t geraete;
  //Local to mirror object mapping
  typedef std::map<GameObject*,GameObject*> objmap_t;
  objmap_t objects;

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

public:
  ///The endpoint of the remote peer
  const Antenna::endpoint endpoint;
  ///The parent NetworkAssembly
  NetworkAssembly*const parent;
  ///The SCG used with this NetworkConnection
  SynchronousControlGeraet*const scg;

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
};

#endif /* NETWORK_CONNECTION_HXX_ */
