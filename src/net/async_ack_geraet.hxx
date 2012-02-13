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
  AAGReceiver(AsyncAckGeraet* aag);

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
  AAGSender(AsyncAckGeraet* aag, NetworkConnection* cxn = NULL);
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
   */
  std::set<seq_t> recentNak;
  std::deque<seq_t> recentNakQueue;

  /* Current set of packets to acknowledge. */
  std::set<seq_t> toAcknowledge;
  /* The greatest (relatively) seq acknowledged in the most recent packet */
  seq_t lastGreatestSeq;

  /* Map outgoing seqs to acknowledgement packet contents;
   * the first element in the pair is the lastGreatestSeq for the
   * packet (not the seq it was sent with).
   */
  std::map<seq_t, std::pair<seq_t, std::set<seq_t> > > pendingAcks;

  /* Time since an ack packet was received. */
  unsigned timeSinceReceive;

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
