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

/* Somehow, the MSVC++ linker can tell the difference between
 * class Peer; declared here and class Peer; declared in
 * net/peer.hxx, and refuses to acknowledge that they are
 * the same type...
 */
#if 0
#ifndef WIN32
class Peer;
#else
#include "../net/peer.hxx"
#endif
#endif

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
   * Implemented in net/peer.hxx.
   */
  //void postRemote(const char*, Peer* = NULL) noth;

  /** Update the deque. The elapsed time is subtracted
   * from the first message's timeLeft; if that results
   * in a negative value, the front message is removed.
   */
  void update(float) noth;

  /** Clears the local messages. */
  void clear() noth;
}

#endif /* GLOBAL_CHAT_HXX_ */
