/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.13
 * @brief Implementation of the Asynchronous Acknowledgement Gerät and friends
 */

#include <map>
#include <set>
#include <deque>

#include "async_ack_geraet.hxx"

using namespace std;

/* The AAG handles its destruction by setting the aag field of all associated
 * senders and receivers to NULL, so both of those classes must gracefully
 * handle this condition.
 */

AAGReceiver::AAGReceiver(AsyncAckGeraet* aag_)
: aag(aag_)
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


AAGSender::AAGSender(AsyncAckGeraet* aag_, NetworkConnection* cxn_)
: OutputNetworkGeraet(cxn_? cxn_ : aag_->cxn),
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


AsyncAckGeraet::AsyncAckGeraet(NetworkConnection* cxn)
: AAGReceiver(this), AAGSender(this, cxn),
  lastGreatestSeq(0), timeSinceReceive(0)
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
  //TODO
}

void AsyncAckGeraet::receiveAccepted(seq_t seq, const byte* data, unsigned len)
throw() {
  //TODO
}

void AsyncAckGeraet::ack(seq_t seq) throw() {
  //TODO
}

void AsyncAckGeraet::nak(seq_t seq) throw() {
  //TODO
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
  toAcknowledge.insert(seq);
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
