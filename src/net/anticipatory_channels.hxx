#ifndef ANTICIPATORY_CHANNEL_HXX_
#define ANTICIPATORY_CHANNEL_HXX_

#include <deque>
#include <map>

#include "network_connection.hxx"

/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.09
 * @brief Contains the AnticipatoryChannels system.
 */

/**
 * Reserves certain channel types in a connection so that they are instantly
 * available when needed.
 *
 * Channel creation using the Synchronous Control Gerät is slow by design. While
 * this is acceptable for most Geräte in the networking system, it is not for
 * short-lived objects, which need to be able to push data down their channel
 * as soon as possible.
 *
 * This class provides an openChannel() function like that of the SCG. If any
 * channels have already been reserved, one of those is transmogrified and
 * returned. If none is reserved, the channel is opened using the SCG directly,
 * and the number to reserve is increased.
 */
class AnticipatoryChannels {
  friend class AnticipatoryChannelsDummy;

  NetworkConnection*const cxn;

  struct Channel {
    unsigned desiredCount;
    unsigned pending;
    std::deque<NetworkConnection::channel> open;

    Channel() : desiredCount(0), pending(0) {}
    Channel(const Channel& c)
    : desiredCount(c.desiredCount), pending(c.pending), open(c.open)
    { }
  };
  std::map<NetworkConnection::geraet_num, Channel> channels;

public:
  /**
   * Creates a new AnticipatoryChannels instance operating on the given
   * NetworkConnection.
   */
  AnticipatoryChannels(NetworkConnection*);

  /**
   * Opens and returns a new channel, either by transmogrifying a reserved
   * channel or by falling back to the SCG.
   */
  NetworkConnection::channel openChannel(OutputNetworkGeraet*,
                                         NetworkConnection::geraet_num)
  throw();

private:
  void reserve(NetworkConnection::geraet_num, Channel&) throw();
  void openned(NetworkConnection::geraet_num, NetworkConnection::channel)
  throw();
};

#endif /* ANTICIPATORY_CHANNEL_HXX_ */
