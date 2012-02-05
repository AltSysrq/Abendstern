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
#include "src/exit_conditions.hxx"

using namespace std;

#define STX ((byte)0x02)
#define EOT ((byte)0x04)
#define ACK ((byte)0x06)
#define SYN ((byte)0x22)
#define XON ((byte)0x11)
#define XOF ((byte)0x13)

static const byte applicationName[] = "Abendstern";
//Just assume the hash is zero for now
static const byte protocolHash[16] = {0};

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
  timeSinceTxn += et;
  if (timeSinceTxn > 2*cxn->latency+128) {
    //May need to send SYN or retransmit last packet
    switch (lastPackOutType) {
      case 0:
        //Idle
        if (timeSinceTxn > 1024) {
          //Send SYN
          byte syn[5];
          byte* synp = syn;
          io::write(synp, cxn->seq());
          io::write(synp, channel);
          io::write(synp, SYN);
          cxn->send(syn, sizeof(syn));
          timeSinceTxn = 0;
        }
        break;

      case XON:
        transmitXon();
        break;

      case XOF:
        transmitXof();
        break;

      case STX:
        transmitStx();
        break;
    }
  //If not waiting for ACK or idle, send any XOFs needed
  } else if (!xofsOut.empty()) {
    transmitXof();
  //If no XOFs, send any XONs needed
  } else if (!xonsOut.empty()) {
    transmitXon();
  }
}

void SynchronousControlGeraet::transmitStx() throw() {
  //STX must always have a SEQ of zero
  byte stx[5+sizeof(protocolHash)+sizeof(applicationName)] = {0,0,0,0,STX};
  //For now, assume hash is zero
  memcpy(stx+5, protocolHash, sizeof(protocolHash));
  memcpy(stx+5+sizeof(protocolHash), applicationName, sizeof(applicationName));
  cxn->send(stx, sizeof(stx));
  lastPackOutType = STX;
  lastPackOutSeq = 0;
}

void SynchronousControlGeraet::transmitXon() throw() {
  //Each element in the XON takes 4 bytes, plus 5 byte header,
  //so we have (256-5)/4 = 62 maximum elements
  byte xon[256];
  byte* xonp = xon;
  NetworkConnection::seq_t seq = cxn->seq();
  io::write(xonp, seq);
  io::write(xonp, channel);
  io::write(xonp, XON);

  unsigned i;
  for (i=0; i<xonsOut.size() && xonp+4 < xon+sizeof(xon); ++i) {
    io::write(xonp, xonsOut[i].remoteGeraet);
    io::write(xonp, xonsOut[i].chan);
  }

  cxn->send(xon, xonp-xon);
  lastPackOutType = XON;
  lastPackOutSeq = seq;
  timeSinceTxn = 0;
  lastXonLen = i;
}

void SynchronousControlGeraet::transmitXof() throw() {
  //Each element in the XOF takes 2 bytes, plus 5 byte header,
  //so we have (256-5)/2 = 125 maximum elements
  byte xof[256];
  byte* xofp = xof;
  NetworkConnection::seq_t seq = cxn->seq();
  io::write(xofp, seq);
  io::write(xofp, channel);
  io::write(xofp, XOF);

  unsigned i;
  for (i=0; i<xofsOut.size() && xofp+2 < xof+sizeof(xof); ++i) {
    io::write(xofp, xofsOut[i]);
  }

  cxn->send(xof, xofp-xof);
  lastPackOutType = XOF;
  lastPackOutSeq = seq;
  timeSinceTxn = 0;
  lastXofLen = i;
}

void SynchronousControlGeraet::transmitAck(NetworkConnection::seq_t aseq)
throw() {
  byte ack[5+2];
  byte* ackp = ack;
  NetworkConnection::seq_t seq = cxn->seq();
  io::write(ackp, seq);
  io::write(ackp, channel);
  io::write(ackp, ACK);
  io::write(ackp, aseq);
  cxn->send(ack, sizeof(ack));
}
