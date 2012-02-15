/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.13
 * @brief Implements the Sequential Text Gerät clasess
 */

#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>

#include <SDL.h>

#include "seq_text_geraet.hxx"
#include "io.hxx"

using namespace std;

SeqTextInputGeraet::SeqTextInputGeraet(AsyncAckGeraet* aag,
                                       DeletionStrategy ds)
: AAGReceiver(aag, ds),
  nextMessageSeq(0)
{ }

void SeqTextInputGeraet::receiveAccepted(NetworkConnection::seq_t,
                                         const byte* data, unsigned len)
throw() {
  if (len < 8) {
    cerr << "Warning: Ignoring Sequential Text message of bad length "
         << len << endl;
    return;
  }

  Uint64 seq;
  const byte* end = data+len;
  io::read(data, seq);
  string str((const char*)data, (const char*)end);

  if (pending.count(seq)) {
    cerr << "Warning: Ignoring Sequential Text message with duplicate seq: "
         << seq << endl;
    return;
  }

  pending.insert(make_pair(seq, str));

  //Don't let the remote peer make us use lots of memory --- violate
  //protocol if more than 8192 message accumulate (as this almost
  //certainly indicates malicious intent and not network reordering).
  if (pending.size() >= 8192) {
    nextMessageSeq = pending.begin()->first;
    cerr << "Warning: Sequential Text Geraet violating protocol due to "
            "accumulation of improperly sequenced messages." << endl;
  }

  //Dequeue messages which are ready
  while (nextMessageSeq == pending.begin()->first) {
    receiveText(pending.begin()->second);
    pending.erase(pending.begin());
    ++nextMessageSeq;
  }
}


SeqTextOutputGeraet::SeqTextOutputGeraet(AsyncAckGeraet* aag,
                                         DeletionStrategy ds)
: ReliableSender(aag, ds),
  nextMessageSeq(0)
{ }

void SeqTextOutputGeraet::send(const string& str) throw() {
  vector<byte> data(NetworkConnection::headerSize +
                    sizeof(Uint64) +
                    str.size());
  byte* dout = &data[NetworkConnection::headerSize];
  io::write(dout, nextMessageSeq++);
  memcpy(dout, &str[0], str.size());
  ReliableSender::send(&data[0], data.size());
}
