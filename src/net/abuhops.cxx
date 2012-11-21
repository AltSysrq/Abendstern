#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <cstdlib>

#include "antenna.hxx"
#include "tuner.hxx"
#include "abuhops.hxx"

namespace abuhops {
  #define MIN_TIME_BETWEEN_WHOAMI 1024
  #define MAX_TIME_BETWEEN_WHOAMI 4096
  #define MIN_TIME_BETWEEN_PING 16384
  #define MAX_TIME_BETWEEN_PING 32768

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

  static unsigned timeUntilPing = 0;
  static bool isConnected = false;
  static bool knowIpv4Address = false, knowIpv6Address = false;

  static void (*listCallback)(void*, const unsigned*, unsigned) = NULL;
  static void* listCallbackUserdata = NULL;

  void connect(unsigned id, const char* name,
               unsigned timestamp, const char* hmac) {
    //TODO
  }

  void ensureRegistered() {
    //TODO
  }

  void bye() {
    //TODO
  }

  void post(const unsigned char* dat, unsigned len) {
    //TODO
  }

  void list(void (*callback)(void*, const unsigned char*, unsigned),
            void* userdata) {
    //TODO
  }

  void stopList() {
    //TODO
  }

  void proxy(const Antenna::endpoint& dst,
             const unsigned char* dat, unsigned len) {
    //TODO
  }

  bool ready() {
    //TODO
    return false;
  }

  void update(unsigned et) {
    //TODO
  }
}
