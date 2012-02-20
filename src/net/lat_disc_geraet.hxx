/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.19
 * @brief Contains the Latency Discovery Gerät (LatDiscGeraet)
 */
#ifndef LAT_DISC_GERAET_HXX_
#define LAT_DISC_GERAET_HXX_

#include <SDL.h>

#include "network_geraet.hxx"

/**
 * Periodically sends PING requests, and answers PINGs with PONGs, to the remote
 * peer in order to determine the average round-trip time between the two peers.
 */
class LatDiscGeraet: public InputNetworkGeraet, public OutputNetworkGeraet {
  Uint32 timeSincePing, signature;

public:
  /** The Gerät number */
  static const NetworkConnection::geraet_num num;
  /** Returns cxn->ldg */
  static InputNetworkGeraet* creator(NetworkConnection* cxn);

  ///Constructs a LatDiscGeraet for the given NetworkConnection
  LatDiscGeraet(NetworkConnection*);
  virtual void update(unsigned) throw();
  virtual void receive(NetworkConnection::seq_t, const byte*, unsigned) throw();
};

#endif /* LAT_DISC_GERAET_HXX_ */
