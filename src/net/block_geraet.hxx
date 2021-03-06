/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.19
 * @brief Contains the base classes for Block Geräte.
 */
#ifndef BLOCK_GERAET_HXX_
#define BLOCK_GERAET_HXX_

#include <vector>
#include <map>
#include <list>
#include <valarray>

#include <SDL.h>

#include "network_geraet.hxx"
#include "async_ack_geraet.hxx"

///Type to use for sequence numbers of block Geräte
typedef Uint32 block_geraet_seq;

/**
 * Base class for the receiving-end of block Geräte.
 * @see newnetworking.txt for details on block Geräte
 */
class InputBlockGeraet: public AAGReceiver {
  block_geraet_seq lastSeq;
  //Map sequence numbers to vectors of packets that are part of that operation;
  //empty packets indicating not-yet-received.
  typedef std::map<block_geraet_seq, std::vector<std::vector<byte> > >
      frags_t;
  frags_t frags;

protected:
  /**
   * The current state of the Gerät. It should not be modified by the subclass.
   */
  std::vector<byte> state;
  /**
   * Bits within are set to 1 whenever a byte at that index in state is modified
   * to a different value by the remote peer.
   *
   * Initialised to all 1s.
   *
   * The subclass must clear bits in this bitset if it wishes to use it.
   */
  std::vector<bool> dirty; //(vector has a specialisation for <bool>)

  /**
   * Constructs an InputBlockGeraet of the given size and on the given AAG.
   */
  InputBlockGeraet(unsigned, AsyncAckGeraet*,
                   InputNetworkGeraet::DeletionStrategy ds = DSNormal);

public:
  virtual void receiveAccepted(NetworkConnection::seq_t,
                               const byte*, unsigned)
  throw();

protected:
  /**
   * Called whenever the remote peer alters the state.
   */
  virtual void modified() throw() {};
};

/**
 * Base class for the transmitting-end of block Geräte.
 * @see newnetworking.txt for details on block Geräte
 */
class OutputBlockGeraet: public AAGSender {
  block_geraet_seq nextSeq;

  /* Possible states that the remote peer may know about, excluding the base
   * state. The bit-set (valarray<unsigned char>) in each element indicates
   * whether each byte was modified by that mutation.
   */
  typedef std::map<block_geraet_seq,
                   std::pair<std::vector<byte>,
                             std::valarray<unsigned char> > >
  remoteStates_t;
  remoteStates_t remoteStates;

  /* If waiting on synchronous mode, this contains packets (including the to-be-
   * written headers) that are pending acknowledgement.
   * If empty, in high-speed mode.
   */
  typedef std::map<NetworkConnection::seq_t, std::vector<byte> > syncPending_t;
  syncPending_t syncPending;

  /* To prevent collapse due to very large updates, limit the number of
   * outstanding synchronous packets and store the rest here.
   */
  std::list<vector<byte> > syncQueue;

  /* Maps network seqs to the block seq they contain. */
  typedef std::map<NetworkConnection::seq_t, block_geraet_seq> pending_t;
  pending_t pending;

  /* The latest state the remote peer is guaranteed to have. */
  std::vector<byte> old;
  /* Whether each byte in the state has been modified by a pending
   * (unacknowledged) state. Such bytes must be considered modified when
   * calculating deltata.
   *
   * More specifically, this is a count of modifications performed by pending
   * mutations; whenever a mutation is ACKed, NAKed, or obsoleted, its
   * concurrent modification count must be subtracted from this value.
   */
  std::valarray<unsigned char> concurrentModifications;

protected:
  /**
   * The current state of the block device. The subclass modifies this to
   * communicate with the remote peer.
   *
   * Note that the size of this vector must NOT be modified.
   *
   * @see dirty
   */
  std::vector<byte> state;
  /**
   * If set to true, the next call to update() will notice changes in the state
   * and will send deltata to the remote peer at the next possible time (which
   * may not be immediate if waiting in synchronous mode).
   *
   * This is reset to false after changes are sent to the remote peer
   * (immediately --- it is not altered when the changes are confirmed).
   */
  bool dirty;

  /**
   * Constructs an OutputBlockGeraet of the given size and on the given AAG.
   *
   * The state is initialised to zero.
   */
  OutputBlockGeraet(unsigned, AsyncAckGeraet*,
                    OutputNetworkGeraet::DeletionStrategy ds = DSNormal);

public:
  virtual void update(unsigned) throw();

  /**
   * Returns true if the local and remote states are confirmed synchronised.
   */
  bool isSynchronised() const throw();

protected:
  virtual void ack(NetworkConnection::seq_t) throw();
  virtual void nak(NetworkConnection::seq_t) throw();
};

#endif /* BLOCK_GERAET_HXX_ */
