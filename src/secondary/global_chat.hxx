/**
 * @file
 * @author Jason Lingle
 * @brief Contains the global text-messaging system (global_chat)
 */

/*
 * global_chat.hxx
 *
 *  Created on: 28.02.2011
 *      Author: jason
 */

#ifndef GLOBAL_CHAT_HXX_
#define GLOBAL_CHAT_HXX_

#include <string>
#include <deque>

#include "src/opto_flags.hxx"

/** The global_chat namespace contains functions that manage
 * the global deque of chat messages.
 */
namespace global_chat {
  /** Defines the properties of a message.
   */
  struct message {
    /** Text to display */
    std::string text;
    /** Milliseconds until removal */
    float timeLeft;
    /** Sequence number */
    unsigned seq;
  };

  /** All messages currently pending or being shown. */
  extern std::deque<message> messages;

  /** Posts a new message, both locally and remotely. */
  void post(const char*) noth;

  /** Posts a message only locally. */
  void postLocal(const char*) noth;

  /** Posts a message only remotely.
   * Does nothing if the network is not running.
   * If no Peer is specified, it defaults to NULL,
   * which indicates to send to all Peers.
   */
  void postRemote(const char*) noth;

  /** Update the deque. The elapsed time is subtracted
   * from the first message's timeLeft; if that results
   * in a negative value, the front message is removed.
   */
  void update(float) noth;

  /** Clears the local messages. */
  void clear() noth;

  /**
   * Defines an object that can transmit messages to remote peers.
   * It is implicitly added to the list of Remotes on construction and
   * removed on destruction.
   */
  class Remote {
    friend void postRemote(const char*) noth;
    Remote(const Remote&); ///< Not implemented

  protected:
    ///Default contsructor
    Remote();

    /**
     * Called when the given message is to be transmitted to remote peers.
     */
    virtual void put(const char*) noth = 0;

  public:
    virtual ~Remote();
  };
}

#endif /* GLOBAL_CHAT_HXX_ */
