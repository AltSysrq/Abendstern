/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.05
 * @brief Implementation of the SynchronousControlGeraet
 */

#include <iostream>
#include <cstdlib>
#include <cassert>

#include "synchronous_control_geraet.hxx"
#include "io.hxx"

using namespace std;

#define STX ((byte)0x02)
#define EOT ((byte)0x04)
#define ACK ((byte)0x06)
#define SYN ((byte)0x22)
#define XON ((byte)0x11)
#define XOF ((byte)0x13)

SynchronousControlGeraet::SynchronousControlGeraet(NetworkConnection* cxn_,
                                                   bool incomming)
: lastXonLen(0),
  lastXofLen(0),
//If outgoing, set timeSinceTxn to a high value so we "retransmit"
//the STX on the next call to update().
  timeSinceTxn(incomming? 0 : 99999),
  lastPackOutSeq(0),
  lastPackOutType(STX),
  cxn(cxn_)
{
  setChannel(0);
  for (NetworkConnection::channel chan=1; chan != 0; ++chan)
    freeChannels.push_back(chan);
}

SynchronousControlGeraet::~SynchronousControlGeraet() {
  //Free OutputNetworkGeraete that were pending opening
  for (unsigned i=0; i<xonsOut.size(); ++i)
    delete xonsOut[i].localGeraet;
}

NetworkConnection::channel
SynchronousControlGeraet::openChannel(OutputNetworkGeraet* localGeraet,
                                      NetworkConnection::geraet_num number)
noth {
  assert(!freeChannels.empty());
  NetworkConnection::channel chan = freeChannels.front();
  freeChannels.pop_front();

  XonDat dat = { chan, number, localGeraet };
  xonsOut.push_back(dat);
  return chan;
}

void SynchronousControlGeraet::closeChannel(NetworkConnection::channel chan)
noth {
  OutputNetworkGeraet* geraet = cxn->outchannels[chan];
  cxn->outchannels.erase(cxn->outchannels.find(chan));
  delete geraet;
  xofsOut.push_back(chan);
}

void SynchronousControlGeraet::receive(NetworkConnection::seq_t seq,
                                       const byte* data, unsigned len)
throw() {
  //TODO: something
}

void SynchronousControlGeraet::update(unsigned et) throw() {
  //TODO: something
}
