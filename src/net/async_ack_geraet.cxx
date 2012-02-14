/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.13
 * @brief Implementation of the Asynchronous Acknowledgement Gerät and friends
 */

#include <map>
#include <set>
#include <deque>
#include <iostream>
#include <cstring>

#include "async_ack_geraet.hxx"
#include "io.hxx"

using namespace std;

/* The AAG handles its destruction by setting the aag field of all associated
 * senders and receivers to NULL, so both of those classes must gracefully
 * handle this condition.
 */

AAGReceiver::AAGReceiver(AsyncAckGeraet* aag_, DeletionStrategy ds)
: InputNetworkGeraet(ds),
  aag(aag_)
{
  aag->assocReceiver(this);
}

AAGReceiver::~AAGReceiver() {
  aag->disassocReceiver(this);
}

void AAGReceiver::receive(NetworkConnection::seq_t seq,
                          const byte* data, unsigned len)
throw() {
  if (aag && aag->incomming(seq)) {
    receiveAccepted(seq, data, len);
  }
}


AAGSender::AAGSender(AsyncAckGeraet* aag_,
                     DeletionStrategy ds,
                     NetworkConnection* cxn_)
: OutputNetworkGeraet(cxn_? cxn_ : aag_->cxn, ds),
  aag(aag_)
{
  aag->assocSender(this);
}

AAGSender::~AAGSender() {
  if (aag)
    aag->disassocSender(this);
}

NetworkConnection::seq_t AAGSender::send(byte* data, unsigned len)
throw() {
  byte* tmp = data;
  NetworkConnection::seq_t seq = cxn->writeHeader(tmp, channel);

  cxn->send(data, len);

  // If no aag exists, we will presumably be deleted soon, so just
  // continue and send the packet without entering it into the
  // acknowledgement system.
  if (aag)
    aag->add(seq, this);

  return seq;
}

void AAGSender::forget(NetworkConnection::seq_t seq) throw() {
  if (aag)
    aag->remove(seq);
}


ReliableSender::ReliableSender(AsyncAckGeraet* aag,
                               DeletionStrategy ds)
: AAGSender(aag, ds),
  nextId(0)
{ }

unsigned ReliableSender::send(byte* data, unsigned len) throw() {
  NetworkConnection::seq_t seq = AAGSender::send(data, len);
  unsigned id = nextId++;
  pending.insert(make_pair(seq, make_pair(id, vector<byte>(data, data+len))));
  return id;
}

void ReliableSender::ack(NetworkConnection::seq_t seq) throw() {
  delivered(pending[seq].first);
  pending.erase(seq);
}

void ReliableSender::nak(NetworkConnection::seq_t seq) throw() {
  pair<unsigned, vector<byte> >& item = pending[seq];
  NetworkConnection::seq_t ns = AAGSender::send(&item.second[0],
                                                item.second.size());
  pending[ns] = item;
  pending.erase(seq);
}


AsyncAckGeraet::AsyncAckGeraet(NetworkConnection* cxn)
: AAGReceiver(this, InputNetworkGeraet::DSNormal),
  AAGSender(this, OutputNetworkGeraet::DSNormal, cxn),
  lastGreatestSeq(0)
{
}

AsyncAckGeraet::~AsyncAckGeraet() {
  //Notify all receivers and senders that we no longer exist
  for (set<AAGReceiver*>::const_iterator it = receivers.begin();
       it != receivers.end(); ++it)
    (*it)->aag = NULL;
  for (set<AAGSender*>::const_iterator it = senders.begin();
       it != senders.end(); ++it)
    (*it)->aag = NULL;
}

void AsyncAckGeraet::update(unsigned et) throw() {
  //Send acknowledgement packet if there is anything to acknowledge
  if (!toAcknowledge.empty()) {
    //Update recent queue
    for (set<seq_t>::const_iterator it = toAcknowledge.begin();
         it != toAcknowledge.end(); ++it)
      recentQueue.push_back(*it);
    //Remove stale entries
    while (recentQueue.size() >= 1024) {
      recentNak.erase(recentQueue.front());
      recentQueue.pop_front();
    }

    sendAck(toAcknowledge, lastGreatestSeq);
    //Record the packets we NAKed
    seq_t ln = lastGreatestSeq;
    for (set<seq_t>::const_iterator it = toAcknowledge.begin();
         it != toAcknowledge.end(); ++it) {
      seq_t curr = *it;
      for (seq_t i = ln+1; i != curr; ++i) {
        //Add this one
        recentNak.insert(i);
      }
    }

    //Clear current acks
    lastGreatestSeq = 1 + *toAcknowledge.rbegin();
    toAcknowledge.clear();
  }
}

void AsyncAckGeraet::sendAck(const set<seq_t>& ack, seq_t base) noth {
  //The length is <header>+<seq>+(7+lastAck)/8
  //(The +7 to effect a round-up)
  vector<byte> datavec(NetworkConnection::headerSize+sizeof(seq_t) +
                        (7+*toAcknowledge.rbegin())/8, 0);
  byte* data = &datavec[4];
  io::write(data, base);

  // Set all positive bits in the bitset
  for (set<seq_t>::const_iterator it = ack.begin(); it != ack.end(); ++it) {
    seq_t s = *it;
    data[s >> 3] |= (1 << (s & 7));
  }

  //Transmit and store for later
  seq_t outseq = send(&datavec[0], datavec.size());
  pendingAcks[outseq] = make_pair(base, ack);
}

void AsyncAckGeraet::receiveAccepted(seq_t seq, const byte* data, unsigned len)
throw() {
  //The length must be at least 3
  if (len < 3) {
    cerr << "Warning: Ignoring ack of invalid length " << len
         << " from source " << cxn->endpoint << endl;
    return;
  }

  seq_t base, lastAcked;
  io::read(data, base);
  lastAcked = base;
  len -= 2;

  //Positive acknowledgements
  for (seq_t suboff = 0; suboff < 8*len; ++suboff) {
    if ((data[suboff>>3] >> (suboff&7)) & 1) {
      //Positive acknowledgement
      seq_t s = base+suboff;
      lastAcked = s;

      pendingOut_t::iterator it = pendingOut.find(s);
      if (it != pendingOut.end()) {
        //It is a packet we were waiting for status on.
        //Copy the sender and ack it after removing to
        //avoid the possibility of concurrent modification
        AAGSender* sender = it->second;
        pendingOut.erase(it);
        sender->ack(s);
      }
    }
  }

  //Scan for negative acknowledgements.
  for (seq_t suboff = 0; suboff+base != lastAcked; ++suboff) {
    if (!((data[suboff>>3] >> (suboff&7)) & 1)) {
      //Negative acknowledgement for relavent packet
      seq_t s = base+suboff;

      pendingOut_t::iterator it = pendingOut.find(s);
      if (it != pendingOut.end()) {
        //Packet waiting for status
        AAGSender* sender = it->second;
        pendingOut.erase(it);
        sender->nak(s);
      }
    }
  }
}

void AsyncAckGeraet::ack(seq_t seq) throw() {
  //ACK received successfully, remove it from storage
  pendingAcks.erase(seq);
}

void AsyncAckGeraet::nak(seq_t seq) throw() {
  //ACK not received, retransmit
  const pair<seq_t, set<seq_t> >& pack = pendingAcks[seq];
  sendAck(pack.second, pack.first);
}

void AsyncAckGeraet::add(seq_t seq, AAGSender* sender) noth {
  pendingOut.insert(make_pair(seq, sender));
}

void AsyncAckGeraet::remove(seq_t seq) noth {
  pendingOut.erase(seq);
}

bool AsyncAckGeraet::incomming(seq_t seq) noth {
  // Make sure we didn't send a NAK for this packet
  if (recentNak.count(seq)) return false;

  // Accept it
  toAcknowledge.insert(seq+lastGreatestSeq);
  return true;
}

void AsyncAckGeraet::assocReceiver(AAGReceiver* r) noth {
  // If r==static_cast<AAGReceiver*>(this), this function is being
  // called from our base class's constructor, and the receivers
  // set is not yet initialised.
  // In this case, simply do nothing, as it is not important to
  // have this in the set.
  if (r != static_cast<AAGReceiver*>(this))
    receivers.insert(r);
}

void AsyncAckGeraet::disassocReceiver(AAGReceiver* r) noth {
  // If r==static_cast<AAGReceiver*>(this), the receivers set
  // has already been destructed, and this function is being
  // called from our base class's destructor.
  // Since this is never placed within the set, simply do nothing
  // in this cgse.
  if (r != static_cast<AAGReceiver*>(this))
    receivers.erase(r);
}

void AsyncAckGeraet::assocSender(AAGSender* s) noth {
  // See notes in assocReciever()
  if (s != static_cast<AAGSender*>(this))
    senders.insert(s);
}

void AsyncAckGeraet::disassocSender(AAGSender* s) noth {
  if (s != static_cast<AAGSender*>(this))
    senders.erase(s);
}
