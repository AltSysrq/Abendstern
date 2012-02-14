/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.13
 * @brief Contains the Sequential Text Gerät classes
 */
#ifndef SEQ_TEXT_GERAET_HXX_
#define SEQ_TEXT_GERAET_HXX_

#include <map>
#include <string>

#include <SDL.h>

#include "async_ack_geraet.hxx"

/**
 * The SeqTextInputGeraet receives data from a SeqTextOutputGeraet,
 * guaranteeing proper ordering of the messages.
 */
class SeqTextInputGeraet: public AAGReceiver {
  Uint64 nextMessageSeq;
  typedef std::map<Uint64, std::string> pending_t;
  pending_t pending;

public:
  /**
   * Constructs a SeqTextInputGeraet for the given AAG and with the
   * given DeletionStrategy, which defaults to DSNormal.
   */
  SeqTextInputGeraet(AsyncAckGeraet*,
                     InputNetworkGeraet::DeletionStrategy ds = DSNormal);

protected:
  virtual void receiveAccepted(NetworkConnection::seq_t,
                               const byte*, unsigned)
  throw();

  /**
   * Called whenever a message is received and is ready to process
   * (ie, it is the immediate next message).
   */
  virtual void receiveText(const std::string&) noth = 0;
};

/**
 * The SeqTextOutputGeraet sends messages to a SeqTextInputGeraet,
 * which guarantees the proper ordering of the messages.
 */
class SeqTextOutputGeraet: public ReliableSender {
  Uint64 nextMessageSeq;
public:
  /**
   * Creates a SeqTextOutputGeraet for the given AAG and with the given
   * DeletionStrategy, which defaults to DSNormal.
   */
  SeqTextOutputGeraet(AsyncAckGeraet*,
                      OutputNetworkGeraet::DeletionStrategy ds = DSNormal);

  /**
   * Sends the given message to the remote peer.
   */
  void send(const std::string&) throw();
};

#endif /* SEQ_TEXT_GERAET_HXX_ */
