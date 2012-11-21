#ifndef ABUHOPS_HXX_
#define ABUHOPS_HXX_

#include "antenna.hxx"

namespace abuhops {
  /**
   * Connects to the Abuhops server on both IPv4 and IPv6, as the host's
   * ability permits.
   *
   * @param id The client ID for the network
   * @param name The client display name for the network
   * @param timestamp Authorisation timestamp issued by the server
   * @param hmac Authorisation HMAC provided by the server, in hexadecimal
   */
  void connect(unsigned id,
               const char* name,
               unsigned timestamp,
               const char* hmac);

  /**
   * Ensures that the Tuners of both standard Antennae are registered with the
   * Abuhops client, so that it will receive transmissions from the server.
   *
   * This must be called whenever the Tuner for either Antenna is changed.
   */
  void ensureRegistered();

  /**
   * Disconnects from the Abuhops server.
   */
  void bye();

  /**
   * POSTs the given raw data to the Abuhops server.
   */
  void post(const unsigned char*, unsigned);
  /**
   * Begins LISTing postings on Abuhops, calling the given function for each
   * packet (where the data is the payload of each ADVERT).
   */
  void list(void (*)(void* userdata, const unsigned char*, unsigned),
            void* userdata);

  /**
   * Sends the given packet to the given destination via a PROXY triangular
   * routing through Abuhops.
   */
  void proxy(const Antenna::endpoint&,
             const unsigned char*, unsigned);

  /**
   * Returns whether Abuhops is ready. That is, whether it has found out the
   * externally-visible address/port of the local host.
   */
  bool ready();

  /**
   * Performs any time-based updates necessary.
   *
   * @param et The time, in milliseconds, since the last call to update().
   */
  void update(unsigned et);
}

#endif /* ABUHOPS_HXX_ */
