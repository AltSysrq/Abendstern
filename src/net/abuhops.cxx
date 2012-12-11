#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <asio.hpp>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "antenna.hxx"
#include "tuner.hxx"
#include "io.hxx"
#include "packet_processor.hxx"
#include "abuhops.hxx"

using namespace std;

namespace abuhops {
  #define MIN_TIME_BETWEEN_WHOAMI 1024
  #define MAX_TIME_BETWEEN_WHOAMI 4096
  #define MIN_TIME_BETWEEN_PING 16384
  #define MAX_TIME_BETWEEN_PING 32768
  #define TIME_BETWEEN_CONNECT 1024
  #define MAX_CONNECT_ATTEMPTS 16

  #define CONNECT 0
  #define PING 1
  #define PROXY 2
  #define POST 3
  #define LIST 4
  #define SIGN 5
  #define BYE 6

  #define YOUARE 0
  #define PONG 1
  #define FROMOTHER 2
  #define ADVERT 3
  #define SIGNATURE 5

  #define IPV4PORT 12545
  #define IPV6PORT 12546
  #define SERVER "ABENDSTERN.SERVEGAME.COM."

  #define HMAC_SIZE 32
  #define MAX_NAME_LENGTH 128

  #ifdef DEBUG
  #define debug(x) cout << "abuhops: " << x << endl;
  #else
  #define debug(x)
  #endif

  static unsigned timeUntilPing = 0;

  static bool isConnected4 = false,  isConnected6 = false, isConnecting = true;
  static unsigned timeUntilConnectXmit = 0;
  static byte connectPacket[1+4+4+HMAC_SIZE+MAX_NAME_LENGTH+1];
  static unsigned connectPacketSize;
  static unsigned connectionAttempts = 0;

  static bool knowIpv4Address = false, knowIpv6Address = false;

  static Antenna::endpoint server4, server6;
  static bool hasv4 = false, hasv6 = false;
  static bool hasInit = false;

  static void sendConnectPacket();
  static void processPacket(bool v6, const byte* data, unsigned len);

  void connect(unsigned id, const char* name,
               unsigned timestamp, const char* hmac) {
    debug(">> CONNECT");
    if (!hasInit) {
      hasInit = true;

      try {
        //Look server IP address up
        asio::io_service svc;
        asio::ip::udp::resolver resolver(svc);
        asio::ip::udp::resolver::query query(SERVER, "0");
        asio::ip::udp::resolver::iterator it(resolver.resolve(query)), end;
        while (it != end && (!hasv4 || !hasv6)) {
          asio::ip::address addr(it++->endpoint().address());
          if (addr.is_v4() && !hasv4) {
            hasv4 = true;
            server4 = Antenna::endpoint(addr, IPV4PORT);
            cout << "Resolved " << SERVER << " ipv4 to " << addr.to_string()
                 << endl;
          } else if (addr.is_v6() && !hasv6) {
            hasv6 = true;
            server6 = Antenna::endpoint(addr, IPV6PORT);
            cout << "Resolved " << SERVER << " ipv6 to " << addr.to_string()
                 << endl;
          }
        }
      } catch (const asio::system_error& err) {
        cerr << "Error resolving " << SERVER << ": " << err.what() << endl;
      }

      //Even if we did get a result for a certain protocol, we can't use it
      //if it is not available
      hasv4 &= antenna.hasV4();
      hasv6 &= antenna.hasV6();

      ensureRegistered();
    }

    //Set initial conditions
    isConnected4 = isConnected6 = false;
    isConnecting = true;
    timeUntilConnectXmit = 0;
    connectionAttempts = 0;

    //Write the connection packet
    byte* pack = connectPacket;
    *pack++ = CONNECT;
    io::write(pack, id);
    io::write(pack, timestamp);
    //Convert HMAC from hex
    for (unsigned i = 0; i < HMAC_SIZE; ++i) {
#define HEXIT(x) (x >= '0' && x <= '9'? x-'0' : \
                  x >= 'a' && x <= 'f'? x-'a'+0xa : x-'A'+0xA)
      *pack++ = (HEXIT(hmac[2*i])<<4) | HEXIT(hmac[2*i+1]);
#undef HEXIT
    }
    //Copy name into packet.
    strncpy((char*)pack, name, MAX_NAME_LENGTH);
    pack[MAX_NAME_LENGTH] = 0;

    //Set packet size
    connectPacketSize = (pack - connectPacket) + strlen(name)+1;

    //Done, send the first packet immediately
    sendConnectPacket();
  }

  static void sendConnectPacket() {
    debug(">> CONNECT (send)");
    if (hasv4 && !isConnected4)
      antenna.send(server4, connectPacket, connectPacketSize);
    if (hasv6 && !isConnected6)
      antenna.send(server6, connectPacket, connectPacketSize);

    ++connectionAttempts;
    if (connectionAttempts > MAX_CONNECT_ATTEMPTS) {
      //Give up, the server probably is not reachable on one or both protocols
      hasv4 &= isConnected4;
      hasv6 &= isConnected6;
      isConnecting = false;
    }

    //If we have not given up or connected yet, try again in a little bit
    timeUntilConnectXmit = TIME_BETWEEN_CONNECT;
  }

  void ensureRegistered() {
    class PP: public PacketProcessor {
    public:
      bool v6;
      PP(bool v) : v6(v) {}
      virtual void process(const Antenna::endpoint&, Antenna*, Tuner*,
                           const byte* data, unsigned len) {
        processPacket(v6, data, len);
      }
    } static ppv4(false), ppv6(true);

    if (antenna.tuner) {
      if (hasv4)
        antenna.tuner->connect(server4, &ppv4);
      if (hasv6)
        antenna.tuner->connect(server6, &ppv6);
    }
  }

  void bye() {
    debug(">> BYE");
    byte pack = BYE;
    if (hasv4)
      antenna.send(server4, &pack, 1);
    if (hasv6)
      antenna.send(server6, &pack, 1);

    isConnecting = isConnected4 = isConnected6 = false;
  }

  void post(bool v6, const byte* dat, unsigned len) {
    debug(">> POST");
    vector<byte> pack(1 + len);
    pack[0] = POST;
    memcpy(&pack[1], dat, len);

    if (isConnected4 && !v6)
      antenna.send(server4, &pack[0], pack.size());
    if (isConnected6 && v6)
      antenna.send(server6, &pack[0], pack.size());
  }

  void list() {
    debug(">> LIST");

    byte pack = LIST;
    if (isConnected4)
      antenna.send(server4, &pack, 1);
    if (isConnected6)
      antenna.send(server6, &pack, 1);
  }

  void stopList() {
  }

  void proxy(const Antenna::endpoint& dst,
             const byte* payload, unsigned len) {
    debug(">> PROXY");
    vector<byte> pack(1 + (dst.address().is_v4()? 4 : 2*8) + 2 + len);
    pack[0] = PROXY;
    byte* dat = &pack[1];
    if (dst.address().is_v4()) {
      asio::ip::address_v4::bytes_type b(dst.address().to_v4().to_bytes());
      memcpy(dat, &b[0], 4);
      dat += 4;
    } else {
      asio::ip::address_v6::bytes_type b(dst.address().to_v6().to_bytes());
      io::a6fromlbo(dat, &b[0]);
      dat += 16;
    }
    io::write(dat, dst.port());
    memcpy(dat, payload, len);

    if (dst.address().is_v4() && isConnected4)
      antenna.send(server4, &pack[0], pack.size());
    else if (dst.address().is_v6() && isConnected6)
      antenna.send(server6, &pack[0], pack.size());
    else
      cerr << "warn: Attempt to proxy via unavailable IP version." << endl;
  }

  bool ready() {
    return hasInit &&
           (!hasv4 || (knowIpv4Address && isConnected4)) &&
           (!hasv6 || (knowIpv6Address && isConnected6));
  }

  void update(unsigned et) {
    if (isConnecting) {
      //Move to non-connecting state if both IP versions we support are
      //connected
      if ((!hasv4 || isConnected4) && (!hasv6 || isConnected6))
        isConnecting = true;
      //Otherwise, retransmit connect packet if necessary
      else if (timeUntilConnectXmit <= et)
        sendConnectPacket();
      else
        timeUntilConnectXmit -= et;
    } else if (isConnected4 || isConnected6) {
      //Send PING if needed
      if (timeUntilPing < et) {
        bool whoAmI = (hasv4 && !knowIpv4Address) ||
                      (hasv6 && !knowIpv6Address);
        byte pack[2] = { PING, (byte)whoAmI };
        if (hasv4)
          antenna.send(server4, pack, 2);
        if (hasv6)
          antenna.send(server6, pack, 2);

        unsigned lower = whoAmI? MIN_TIME_BETWEEN_WHOAMI:MIN_TIME_BETWEEN_PING;
        unsigned upper = whoAmI? MAX_TIME_BETWEEN_WHOAMI:MAX_TIME_BETWEEN_PING;
        timeUntilPing = lower + rand()%(upper-lower);
      } else {
        timeUntilPing -= et;
      }
    }
  }

  static void processYouAre(bool, const byte*, unsigned);
  static void processPong(bool, const byte*, unsigned);
  static void processFromOther(bool, const byte*, unsigned);
  static void processAdvert(bool, const byte*, unsigned);
  static void (*const packetTypes[256])(bool, const byte*, unsigned) = {
    processYouAre,
    processPong,
    processFromOther,
    processAdvert,
    NULL,
    /* processSignature, */
    /* rest of array is NULL implicitly */
  };

  static void processPacket(bool v6, const byte* dat, unsigned len) {
    if (packetTypes[dat[0]])
      packetTypes[dat[0]](v6, dat+1, len-1);
    else {
#ifdef DEBUG
      cerr << "WARN: Dropping unknown abuhops packet type " << (unsigned)dat[0]
           << endl;
#endif
    }
  }

  static void processYouAre(bool v6, const byte* dat, unsigned len) {
    debug("<< YOU-ARE");
    if (v6) {
      GlobalID& gid(*antenna.getGlobalID6());
      if (len != 8*2 + 2) {
#ifdef DEBUG
        cerr << "WARN: Dropping IPv6 YOU-ARE packet of length " << len << endl;
#endif
      } else {
        for (unsigned i = 0; i < 8; ++i)
          io::read(dat, gid.ia6[i]);
        io::read(dat, gid.iport);

        knowIpv6Address = true;
        isConnected6 = true;

        cout << "Our IPv6 GlobalID is " << gid.toString() << endl;
      }
    } else {
      GlobalID& gid(*antenna.getGlobalID4());
      if (len != 4 + 2) {
#ifdef DEBUG
        cerr << "WARN: Dropping IPv4 YOU-ARE packet of length " << len << endl;
#endif
      } else {
        memcpy(gid.ia4, dat, 4);
        dat += 4;
        io::read(dat, gid.iport);

        knowIpv4Address = true;
        isConnected4 = true;

        cout << "Our IPv4 GlobalID is " << gid.toString() << endl;
      }
    }
  }

  static void processPong(bool v6, const byte* dat, unsigned len) {
    //Nothing to do
    debug("<< PONG");
  }

  static void processFromOther(bool v6, const byte* dat, unsigned len) {
    debug("<< FROM-OTHER");
    if (!len) {
#ifdef DEBUG
      cerr << "WARN: Dropping FROM-OTHER with empty payload." << endl;
#endif
      return;
    }

    //The default endpoint is used to tell the NetworkGame that it must get the
    //Internet IP address from the STX itself
    Antenna::endpoint defaultEndpoint;
    if (antenna.tuner)
      antenna.tuner->receivePacket(defaultEndpoint, &antenna, dat, len);
  }

  static const char requiredAdvertHeader[] = "Abendspiel";
  #define ADVERT_IPV_OFF (sizeof(requiredAdvertHeader)-1 + 4 + 1)
  static void processAdvert(bool v6, const byte* dat, unsigned len) {
    debug("<< ADVERT");
    Antenna::endpoint defaultEndpoint;
    if (len < ADVERT_IPV_OFF ||
        memcmp(dat, requiredAdvertHeader, sizeof(requiredAdvertHeader)-1)) {
#ifdef DEBUG
      cerr << "WARN: Dropping non-Abendspiel ADVERT" << endl;
#endif
      return;
    }

    //Ensure that the reported IP version of the advert matches the Abuhops
    //realm it is comming from
    if ((unsigned)v6 != dat[ADVERT_IPV_OFF]) {
      /*
       * This warning is meaningless, since the Abendstern abuhops client
       * always posts to both realms.
#ifdef DEBUG
      cerr << "WARN: Dropping ADVERT for wrong IP version" << endl;
#endif
      */
      return;
    }

    if (antenna.tuner)
      antenna.tuner->receivePacket(defaultEndpoint, &antenna, dat, len);
  }
}
