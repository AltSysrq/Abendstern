/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.19
 * @brief Implements the Latency Discovery Gerät.
 */

#include <cstdlib>

#include <SDL.h>

#include "lat_disc_geraet.hxx"
#include "network_connection.hxx"
#include "io.hxx"

using namespace std;

#define PING 0
#define PONG 1
#define PING_INTERVAL 1024

const NetworkConnection::geraet_num LatDiscGeraet::num =
    NetworkConnection::registerGeraetCreator(&creator, 0);

InputNetworkGeraet* LatDiscGeraet::creator(NetworkConnection* cxn) {
  return cxn->ldg;
}

LatDiscGeraet::LatDiscGeraet(NetworkConnection* cxn)
: InputNetworkGeraet(InputNetworkGeraet::DSIntrinsic),
  OutputNetworkGeraet(cxn, OutputNetworkGeraet::DSIntrinsic),
  timeSincePing(0), signature(rand())
{
}

void LatDiscGeraet::update(unsigned et) throw() {
  timeSincePing += et;

  if (timeSincePing > PING_INTERVAL) {
    //Send new PING
    byte packet[NetworkConnection::headerSize + 1 + sizeof(signature)];
    signature = rand();
    byte* out = packet;
    cxn->writeHeader(out, channel);
    *out++ = PING;
    io::write(out, signature);

    cxn->send(packet, sizeof(packet));

    timeSincePing = 0;
  }
}

void LatDiscGeraet::receive(NetworkConnection::seq_t,
                            const byte* dat, unsigned len)
throw() {
  if (!len) return; //Ignore empty packet

  switch (*dat) {
    case PING: {
      //Answer immediately
      static byte pack[256];

      //Truncate if too long
      if (len > sizeof(pack)-NetworkConnection::headerSize)
        len = sizeof(pack)-NetworkConnection::headerSize;

      //Create and send new PONG packet
      byte* out = pack;
      cxn->writeHeader(out, channel);
      *out++ = PONG;
      memcpy(out, dat+1, len-1);
    } break;

    case PONG: {
      if (len == 1+sizeof(signature)) {
        //Packet has the expected size
        Uint32 sig;
        io::read(dat, sig);

        if (sig == signature) {
          //Matching PONG; move the current latency closer to the latency
          //of this packet
          cxn->latency = (cxn->latency*9 + timeSincePing)/10;
          //Randomise signature so potential duplicates are ignore
          signature = rand();
        }
      }
    } break;
  }
}
