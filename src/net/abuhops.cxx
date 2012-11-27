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

  static unsigned timeUntilPing = 0;

  static bool isConnected4 = false,  isConnected6 = false, isConnecting = true;
  static unsigned timeUntilConnectXmit = 0;
  static byte connectPacket[1+4+4+HMAC_SIZE+MAX_NAME_LENGTH+1];
  static unsigned connectPacketSize;
  static unsigned connectionAttempts = 0;

  static bool knowIpv4Address = false, knowIpv6Address = false;

  static Antenna::endpoint server4, server6;
  static bool hasv4 = false, hasv6 = false;

  static void (*listCallback)(void*, const byte*, unsigned) = NULL;
  static void* listCallbackUserdata = NULL;

  static void sendConnectPacket();
  static void processPacket(bool v6, const byte* data, unsigned len);

  void connect(unsigned id, const char* name,
               unsigned timestamp, const char* hmac) {
    static bool hasInit = false;
    if (!hasInit) {
      hasInit = true;

      try {
        //Look server IP address up
        asio::io_service svc;
        asio::ip::udp::resolver resolver(svc);
        asio::ip::udp::resolver::query query(SERVER);
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
    byte pack = BYE;
    if (hasv4)
      antenna.send(server4, &pack, 1);
    if (hasv6)
      antenna.send(server6, &pack, 1);

    isConnecting = isConnected4 = isConnected6 = false;
  }

  void post(const byte* dat, unsigned len) {
    vector<byte> pack(1 + len);
    pack[0] = POST;
    memcpy(&pack[1], dat, len);

    if (hasv4)
      antenna.send(server4, &pack[0], pack.size());
    if (hasv6)
      antenna.send(server6, &pack[0], pack.size());
  }

  void list(void (*callback)(void*, const byte*, unsigned),
            void* userdata) {
    listCallback = callback;
    listCallbackUserdata = userdata;

    byte pack = LIST;
    if (isConnected4)
      antenna.send(server4, &pack, 1);
    if (isConnected6)
      antenna.send(server6, &pack, 1);
  }

  void stopList() {
    //TODO
  }

  void proxy(const Antenna::endpoint& dst,
             const byte* dat, unsigned len) {
    //TODO
  }

  bool ready() {
    //TODO
    return false;
  }

  void update(unsigned et) {
    //TODO
  }

  static void processPacket(bool v6, const byte* dat, unsigned len) {
    //TODO
  }
}
