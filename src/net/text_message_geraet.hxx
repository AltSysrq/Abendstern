/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.18
 * @brief Contains the Text Message Gerät components
 */
#ifndef TEXT_MESSAGE_GERAET_HXX_
#define TEXT_MESSAGE_GERAET_HXX_

#include "network_geraet.hxx"
#include "async_ack_geraet.hxx"
#include "src/secondary/global_chat.hxx"

/**
 * Sends remotely-transmitted global chat messages to the connected
 * remote peer.
 */
class TextMessageOutputGeraet:
public ReliableSender,
private global_chat::Remote {
public:
  /**
   * Constructs a TextMessageOutputGeraet on the given AAG.
   */
  TextMessageOutputGeraet(AsyncAckGeraet*);

protected:
  virtual void put(const char*) noth;
};

/**
 * Forwards received messages to the global chat system.
 */
class TextMessageInputGeraet: public AAGReceiver {
public:
  ///Constructs a TextMessageInputGeraet on the given AAG.
  TextMessageInputGeraet(AsyncAckGeraet*);

  ///The Gerät number for this class
  static const NetworkConnection::geraet_num num;

protected:
  virtual void receiveAccepted(NetworkConnection::seq_t, const byte*, unsigned)
  throw();

private:
  static InputNetworkGeraet* creator(NetworkConnection*);
};

#endif /* TEXT_MESSAGE_GERAET_HXX_ */
