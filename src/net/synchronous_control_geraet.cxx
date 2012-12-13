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
#include "network_appinfo.hxx"
#include "abuhops.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"

using namespace std;

#define STX ((byte)0x02)
#define EOT ((byte)0x04)
#define ACK ((byte)0x06)
#define SYN ((byte)0x22)
#define XON ((byte)0x11)
#define XOF ((byte)0x13)

SynchronousControlGeraet::SynchronousControlGeraet(NetworkConnection* cxn_,
                                                   bool incomming)
: InputNetworkGeraet(InputNetworkGeraet::DSIntrinsic),
  OutputNetworkGeraet(cxn_, OutputNetworkGeraet::DSIntrinsic),
  lastXonLen(0),
  lastXofLen(0),
  //Set timeSinceTxn to a high value so we "retransmit"
  //the STX on the next call to update().
  timeSinceTxn(99999),
  lastPackOutSeq(0),
  lastPackOutType(STX)
{
  setChannel(0);
  for (NetworkConnection::channel chan=1; chan != 0; ++chan)
    freeChannels.push_back(chan);
}

SynchronousControlGeraet::~SynchronousControlGeraet() {
  //Free OutputNetworkGeraete that were pending opening
  for (unsigned i=0; i<xonsOut.size(); ++i)
    if (xonsOut[i].localGeraet->deletionStrategy !=
        OutputNetworkGeraet::DSIntrinsic)
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
  localGeraet->setChannel(chan);
  return chan;
}

void SynchronousControlGeraet::closeChannel(NetworkConnection::channel chan)
noth {
  OutputNetworkGeraet* geraet = cxn->outchannels[chan];
  assert(geraet->deletionStrategy == OutputNetworkGeraet::DSNormal);
  cxn->outchannels.erase(cxn->outchannels.find(chan));
  delete geraet;
  xofsOut.push_back(chan);
}

void SynchronousControlGeraet::closeConnection(const char* english,
                                               const char* l10n)
noth {
  vector<byte> pack(strlen(english)+strlen(l10n)+5+2);
  byte* data = &pack[0];
  cxn->writeHeader(data, channel);
  io::write(data, EOT);
  io::writes(data, &pack[pack.size()], string(english));
  io::writes(data, &pack[pack.size()], string(l10n));
  cxn->send(&pack[0], pack.size());
  cxn->status = NetworkConnection::Zombie;

  //Look l10n entry up
  cxn->disconnectReason = ::l10n::lookup('N', string("protocol"), string(l10n));
  //Fall back on english if lookup fails
  if (string::npos == cxn->disconnectReason.find_first_not_of("#")) {
    cxn->disconnectReason = english;
  }

  #ifdef DEBUG
  cerr << "Disconnect from peer " << cxn->endpoint << ": "
        << english << " (" << cxn->disconnectReason << ")" << endl;
  #endif
}

void SynchronousControlGeraet::receive(NetworkConnection::seq_t seq,
                                       const byte* data, unsigned len)
throw() {
  if (!len) {
    #ifdef DEBUG
    cerr << "Warning: Received empty packet to Synchronous Control Geraet; "
         << "seq = " << seq << ", source = " << cxn->endpoint << endl;
    #endif
    return;
  }

  switch (*data) {
    default:
      #ifdef DEBUG
      cerr << "Warning: Received unknown message to Synchronous Control Geraet;"
           << " seq = " << seq << ", source = " << cxn->endpoint
           << ", type = " << (int)*data << endl;
      #endif
      return;

    case SYN: break;
    case ACK: {
      if (len != 3) {
        #ifdef DEBUG
        cerr << "Warning: Received invalid ACK to Synchronous Control Geraet;"
             << " seq = " << seq << ", source = "<< cxn->endpoint
             << ", len = " << len << endl;
        #endif
        return;
      }
      ++data;
      NetworkConnection::seq_t acked;
      io::read(data, acked);
      if (acked == lastPackOutSeq) {
        switch (lastPackOutType) {
          case 0:
            //This packet already acknowledged
            break;

          case STX:
            lastPackOutType = 0;
            break;

          case XON: {
            //Geräte activated
            for (unsigned i=0; i<lastXonLen; ++i) {
              const XonDat& dat(xonsOut.front());
              cxn->outchannels[dat.chan] = dat.localGeraet;
              dat.localGeraet->outputOpen();
              xonsOut.pop_front();
            }
            lastPackOutType = 0;
          } break;

          case XOF: {
            //Geräte deactivated
            for (unsigned i=0; i<lastXofLen; ++i) {
              freeChannels.push_back(xofsOut.front());
              xofsOut.pop_front();
            }
            lastPackOutType = 0;
          } break;

          default:
            cerr << "FATAL: Unexpected value of lastPackOutSeq: "
                 << (int)lastPackOutType << endl;
            exit(EXIT_PROGRAM_BUG);
        }
      } else {
        /* An ACK for a packet other than the one we last sent generally
         * indicates that we are not allocating enough time for send-acknowledge
         * round-trips; most likely, we retransmitted a new packet because we
         * assumed the previous was dropped, then we received the ACK for the
         * older one.
         * In essence, treat this as a source quench by adding twice the time
         * differential to the expected latency of the connection.
         */
        cxn->latency += timeSinceTxn*2;
      }
    } break;

    case STX: {
      //Connection established
      if (cxn->status == NetworkConnection::Connecting) {
        cxn->status = NetworkConnection::Established;
        if (len > sizeof(protocolHash) + sizeof(applicationName)+1)
          auxData.assign(data+sizeof(protocolHash)+sizeof(applicationName)+1,
                         data+len);
      }
      //Already validated if the first packet, so just reacknowledge.
      //(Don't bother checking for it during a connection.)
      transmitAck(seq);
    } break;

    case XON: {
      ++data, --len; //Past type information
      if (len % 4) {
        #ifdef DEBUG
        cerr << "Invalid XON length from " << cxn->endpoint
             << ": " << (len+1) << endl;
        #endif
        closeConnection("Invalid XON length", "protocol_error");
        return;
      }

      const byte* end = data+len;
      while (data < end) {
        NetworkConnection::geraet_num num;
        NetworkConnection::channel chan;
        io::read(data, num);
        io::read(data, chan);

        //Silently ignore opens to already-open channels
        if (cxn->inchannels.count(chan)) continue;

        //Get the Gerät creator
        NetworkConnection::geraet_creator creator =
            cxn->getGeraetCreator(num);
        if (!creator) {
          #ifdef DEBUG
          cerr << "Invalid XON Geraet number from " << cxn->endpoint
               << ": " << (unsigned)num << endl;
          #endif
          closeConnection("Unknown Geraet number", "protocol_error");
          return;
        }

        InputNetworkGeraet* geraet = creator(cxn);
        if (!geraet) {
          #ifdef DEBUG
          cerr << "Warning: Ignoring rejected creation of Geraet " << num<<endl;
          #endif
        } else {
          cxn->inchannels[chan] = geraet;
          cxn->inchannels[chan]->inputChannel = chan;
        }
      }

      //Done processing, acknowledge packet
      transmitAck(seq);
    } break;

    case XOF: {
      ++data, --len; //Past type byte
      if (len % 2) {
        #ifdef DEBUG
        cerr << "Invalid XOF length from " << cxn->endpoint
             << ": " << (len+1) << endl;
        #endif
        closeConnection("Invalid XOF length", "protocol_error");
        return;
      }

      const byte* end = data+len;
      while (data < end) {
        NetworkConnection::channel chan;
        io::read(data, chan);

        //Can't close SCG
        if (!chan) {
          #ifdef DEBUG
          cerr << "Closed SCG: " << cxn->endpoint << endl;
          #endif
          closeConnection("Closed SCG", "protocol_error");
          return;
        }

        //Search for the open channel
        NetworkConnection::inchannels_t::iterator it =
            cxn->inchannels.find(chan);
        //Remove if exists; silently ignore if not exists or if
        //it is marked Intrinsic or Eternal
        if (it != cxn->inchannels.end()) {
          if (it->second->deletionStrategy == InputNetworkGeraet::DSNormal) {
            delete it->second;
            cxn->inchannels.erase(it);
          } else {
            cerr << "Warning: Ignoring attempt to close intrinsic or eternal "
                    "Geraet from " << cxn->endpoint << endl;
          }
        }
      }

      //Acknowledge packet
      transmitAck(seq);
    } break;

    case EOT: {
      ++data, --len;
      const byte* end = data+len;
      string english, l10n;
      io::reads(data, end, english);
      io::reads(data, end, l10n);

      //Look l10n entry up
      l10n = ::l10n::lookup('N', string("protocol"), l10n);
      //Fall back on english if lookup fails
      if (string::npos == l10n.find_first_not_of("#")) {
        l10n = english;
      }

      #ifdef DEBUG
      cerr << "Peer " << cxn->endpoint << " disconnected: "
           << english << " (" << l10n << ")" << endl;
      #endif
      cxn->disconnectReason = l10n;
      cxn->status = NetworkConnection::Zombie;
    } break;
  }
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
          cxn->writeHeader(synp, channel);
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
  } else if (!xofsOut.empty() && !lastPackOutType) {
    transmitXof();
  //If no XOFs, send any XONs needed
  } else if (!xonsOut.empty() && !lastPackOutType) {
    transmitXon();
  }
}

void SynchronousControlGeraet::transmitStx() throw() {
  //STX must always have a SEQ of zero
  byte stx[5+sizeof(protocolHash)+sizeof(applicationName)+256] =
    {0,0,0,0,STX};
  //For now, assume hash is zero
  memcpy(stx+5, protocolHash, sizeof(protocolHash));
  memcpy(stx+5+sizeof(protocolHash), applicationName, sizeof(applicationName));
  memcpy(stx+5+sizeof(protocolHash)+sizeof(applicationName),
         &auxDataOut[0], auxDataOut.size());
  cxn->send(stx,
            5+sizeof(protocolHash)+sizeof(applicationName)+auxDataOut.size());
  abuhops::proxy(cxn->endpoint, stx,
                 5+sizeof(protocolHash)+sizeof(applicationName)+
                     auxDataOut.size());
  lastPackOutType = STX;
  lastPackOutSeq = 0;
  timeSinceTxn = 0;
}

void SynchronousControlGeraet::transmitXon() throw() {
  //Each element in the XON takes 4 bytes, plus 5 byte header,
  //so we have (256-5)/4 = 62 maximum elements
  byte xon[256];
  byte* xonp = xon;
  NetworkConnection::seq_t seq = cxn->writeHeader(xonp, channel);
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
  NetworkConnection::seq_t seq = cxn->writeHeader(xofp, channel);
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
  cxn->writeHeader(ackp, channel);
  io::write(ackp, ACK);
  io::write(ackp, aseq);
  cxn->send(ack, sizeof(ack));
}
