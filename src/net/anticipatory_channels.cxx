#include <deque>
#include <map>

#include "anticipatory_channels.hxx"
#include "network_geraet.hxx"
#include "synchronous_control_geraet.hxx"

using namespace std;

/* Dummy output Gerät to hold the channel and notify the parent
 * of when the channel is actually ready.
 */
class AnticipatoryChannelsDummy: public OutputNetworkGeraet {
  AnticipatoryChannels*const parent;
  NetworkConnection::geraet_num num;

public:
  AnticipatoryChannelsDummy(AnticipatoryChannels* par,
                            NetworkConnection::geraet_num num_,
                            NetworkConnection* cxn)
  : OutputNetworkGeraet(cxn),
    parent(par), num(num_)
  { }

  virtual void outputOpen() throw() {
    parent->openned(num, channel);
  }
};

AnticipatoryChannels::AnticipatoryChannels(NetworkConnection* cxn_)
: cxn(cxn_)
{ }

NetworkConnection::channel
AnticipatoryChannels::openChannel(OutputNetworkGeraet* ong,
                                  NetworkConnection::geraet_num num)
throw() {
  Channel& chan = channels[num];
  NetworkConnection::channel ret;
  if (!chan.open.empty()) {
    //Give reserved
    ret = chan.open.front();
    chan.open.pop_front();
    cxn->transmogrify(ret, ong);
  } else {
    //Fall back, then increase reserve
    ret = cxn->scg->openChannel(ong, num);

    //Increase by 25% or 8, whichever is greater,
    //if not already waiting for new channels
    unsigned delta = chan.desiredCount/4;
    if (delta < 8) delta = 8;
    if (chan.pending == chan.desiredCount)
      chan.desiredCount += delta;
  }

  //Reserve new channels
  reserve(num, chan);

  return ret;
}

void AnticipatoryChannels::reserve(NetworkConnection::geraet_num num,
                                   Channel& chan)
throw() {
  while (chan.open.size() + chan.pending < chan.desiredCount) {
    cxn->scg->openChannel(new AnticipatoryChannelsDummy(this, num, cxn), num);
    ++chan.pending;
  }
}

void AnticipatoryChannels::openned(NetworkConnection::geraet_num num,
                                   NetworkConnection::channel ch)
throw() {
  Channel& chan = channels[num];
  --chan.pending;
  chan.open.push_back(ch);
}
