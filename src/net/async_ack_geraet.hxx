/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.12
 * @brief Contains the Asynchronous Acknowledgement Gerät and related classes.
 */
#ifndef ASYNC_ACK_GERAET_HXX_
#define ASYNC_ACK_GERAET_HXX_

#include <map>
#include <set>
#include <deque>
#include <vector>

#include "network_geraet.hxx"

class AsyncAckGeraet;

/**
 * Interfaces an input Gerät with the Asynchronous Acknowledgement Gerät.
 * It transparently handles the dropping of negatively-acknowledged packets,
 * and informing the AAG of packets to acknowledge.
 *
 * Subclasses must override receiveAccepted() instead of receive().
 *
 * @see AsyncAckGeraet
 * @see AAGSender
 */
class AAGReceiver: public InputNetworkGeraet {
  friend class AsyncAckGeraet;

  AsyncAckGeraet* aag;

protected:
  /**
   * Constructs the AAGReceiver to operate on the given AAG.
   */
  AAGReceiver(AsyncAckGeraet* aag,
              InputNetworkGeraet::DeletionStrategy ds = DSNormal);

  /**
   * Called whenever a packet is received and is not being dropped
   * due to prior negative acknowledgement.
   *
   * @see InputNetworkGeraet::receive()
   * @see AAGReceiver::receive()
   */
  virtual void receiveAccepted(NetworkConnection::seq_t seq,
                               const byte* data, unsigned len)
  throw() = 0;

public:
  virtual ~AAGReceiver();

  /**
   * Performs class-specific operations, then calls receiveAccepted().
   * <strong>Do not override this function!</strong>
   * @see receiveAccepted()
   */
  virtual void receive(NetworkConnection::seq_t ,
                       const byte*, unsigned) throw();

};

/**
 * Interfaces an output Gerät with the Asynchronous Acknowledgement Gerät.
 * It exposes the relevent portion of the AAG's private interface to
 * subclasses, allowing them to know the state of their packets.
 *
 * @see AsyncAckGeraet
 * @see AAGReceiver
 */
class AAGSender: public OutputNetworkGeraet {
  friend class AsyncAckGeraet;

  AsyncAckGeraet* aag;

protected:
  /**
   * Constructs an AAGSender operating on the given AAG.
   * If no NetworkConnection is specified, it defaults to
   * the one associated with the AAG.
   */
  AAGSender(AsyncAckGeraet* aag,
            OutputNetworkGeraet::DeletionStrategy ds = DSNormal,
            NetworkConnection* cxn = NULL);
public:
  virtual ~AAGSender();

protected:
  /**
   * Sends the given data to the remote peer, using the AAG
   * to track its status.
   * Unless forget() is used, ack() or nak() will eventually
   * be called to indicate the status of the outgoing packet.
   *
   * Note that the packet header will be written by this function;
   * subclasses should skip the header when writing the packet
   * themselves.
   *
   * @param data the packet data, including the (uninitialised) header
   * @param len the length of data
   * @returns the sequence number of the sent packet
   */
  NetworkConnection::seq_t send(byte* data, unsigned len) throw();

  /**
   * Called when the packet of the given sequence number was positively
   * acknowledged by the remote peer. When this is called, it is known
   * with certainty that the remote peer has received the packet.
   */
  virtual void ack(NetworkConnection::seq_t) throw() = 0;
  /**
   * Called when the packet of the given sequence number was negatively
   * acknowledged by the remote peer. When this is callvd, it is known
   * with certaintay that the remote peer will never receive the packet.
   */
  virtual void nak(NetworkConnection::seq_t) throw() = 0;

  /**
   * Removes the given sequence number from the AAG's list of pending
   * packets, as if it were never sent. Neither ack() nor nak() will
   * be called for this packet.
   */
  void forget(NetworkConnection::seq_t) throw();
};

/**
 * Extends AAGSender to perform the rather common task of delivering
 * packets reliably --- that is, packets which are not received by
 * the destination are retransmitted as necessary.
 *
 * Note that, due to this behaviour, the subclass canNOT rely on using
 * the sequence numbers of packets to identify them; if it needs
 * identification, it must use its own sequence system.
 */
class ReliableSender: public AAGSender {
  std::map<NetworkConnection::seq_t,
           std::pair<unsigned, std::vector<byte> > > pending;
  unsigned nextId;

public:
  /**
   * Constructs a ReliableSender with the given AAG
   * and DeletionStrategy, which defaults to DSNormal.
   */
  ReliableSender(AsyncAckGeraet*,
                 OutputNetworkGeraet::DeletionStrategy ds = DSNormal);

  /**
   * Sends the given packet data, ensuring that the packet is
   * eventually delivered.
   * @returns a (mostly) unique ID identifying this packet internally.
   * The id number will be unique until integer wraparound takes effect.
   */
  unsigned send(byte*, unsigned len) throw();
  /**
   * Notifies the subclass that the packet of the given id has been
   * confirmed delivered.
   *
   * Default does nothing.
   */
  virtual void delivered(unsigned) noth { }

  virtual void ack(NetworkConnection::seq_t) throw();
  virtual void nak(NetworkConnection::seq_t) throw();
};

/**
 * Tracks packets requiring acknowledgement.
 * Clients of this class must use AAGSender and/or AAGReceiver
 * to be able to do anything with it.
 */
class AsyncAckGeraet: public AAGReceiver, public AAGSender {
  friend class AAGReceiver;
  friend class AAGSender;

  typedef NetworkConnection::seq_t seq_t;

  /* Map pending sequence numbers to their senders. */
  typedef std::map<seq_t,AAGSender*> pendingOut_t;
  pendingOut_t pendingOut;

  /* Associated senders and receivers */
  std::set<AAGSender*> senders;
  std::set<AAGReceiver*> receivers;

  /* Recently negatively acknowledged packets, and a queue to quickly
   * remove them in the correct order.
   *
   * All packets ACKed or NAKed are listed in the queue, so that the recentNak
   * set does not have persistent entries on strong links.
   */
  std::set<seq_t> recentNak;
  std::deque<seq_t> recentQueue;

  /* Current set of packets to acknowledge.
   * Sequence numbers are relative to lastGreatestSeq.
   */
  std::set<seq_t> toAcknowledge;
  /* The greatest (relatively) seq acknowledged in the most recent packet */
  seq_t lastGreatestSeq;

  /* Map outgoing seqs to acknowledgement packet contents;
   * the first element in the pair is the lastGreatestSeq for the
   * packet (not the seq it was sent with), and is thus the value
   * all other seqs are relative to.
   */
  typedef std::map<seq_t, std::pair<seq_t, std::set<seq_t> > > pendingAcks_t;
  pendingAcks_t pendingAcks;

public:
  /** The Gerät number for the AAG */
  static const NetworkConnection::geraet_num num = 1;

  /**
   * Constructs an AAG for the given NetworkConnection.
   */
  AsyncAckGeraet(NetworkConnection*);
  virtual ~AsyncAckGeraet();

  virtual void update(unsigned) throw();

protected:
  virtual void receiveAccepted(seq_t, const byte*, unsigned) throw();
  virtual void ack(seq_t) throw();
  virtual void nak(seq_t) throw();


private:
  /* Encodes and transmits an acknowledgement for the given set of
   * sequence numbers and the previous "greatest" sequence number.
   */
  void sendAck(const std::set<seq_t>&, seq_t prevGreatest) noth;

  /* Interface for AAGSender/AAGReceiver */
  /* Adds a packet requiring acknowledgement */
  void add(seq_t, AAGSender*) noth;
  /* Removes a packet by seq */
  void remove(seq_t) noth;
  /* Notifies the AAG of receiving the given packet seq;
   * returns whether it should actually be processed.
   */
  bool incomming(seq_t) noth;

  void assocReceiver(AAGReceiver*) noth;
  void disassocReceiver(AAGReceiver*) noth;
  void assocSender(AAGSender*) noth;
  void disassocSender(AAGSender*) noth;
};

#endif /* ASYNC_ACK_GERAET_HXX_ */
