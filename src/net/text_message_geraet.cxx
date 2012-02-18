/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.18
 * @brief Implementation of the Text Message Gerät.
 */

#include <cstring>

#include "text_message_geraet.hxx"

using namespace std;

TextMessageOutputGeraet::TextMessageOutputGeraet(AsyncAckGeraet* aag)
: ReliableSender(aag)
{ }

void TextMessageOutputGeraet::put(const char* str) {
  static byte buff[1024] = {0};
  strncpy((char*)(buff+NetworkConnection::headerSize), str,
          sizeof(buff)-1-NetworkConnection::headerSize);
  send(buff, strlen(str)+NetworkConnection::headerSize);
}

TextMessageInputGeraet::TextMessageInputGeraet(AsyncAckGeraet* aag)
: AAGReceiver(aag)
{ }

void TextMessageInputGeraet::receiveAccepted(NetworkConnection::seq_t,
                                             const byte* dat, unsigned len)
throw() {
  /* Since the packet is not necessarily NUL-terminated (and usually isn't,
   * since TextMessageOutputGeraet::put() doesn't send the NUL), we need
   * to copy into a temporary buffer first.
   *
   * Initialise the buffer to zero at static load time, then strncpy() up to
   * one less than its length into the buffer (obviously only copying up to
   * the length of the packet).
   *
   * Then post the buffer to local global chat.
   */
  static char buff[1024] = {0};
  if (len > sizeof(buff)-1)
    len = sizeof(buff)-1;
  strncpy(buff, (const char*)dat, len);

  global_chat::postLocal(buff);
}
