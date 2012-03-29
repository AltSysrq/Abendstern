/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.01
 * @brief Contains the ConnectionListener packet processing class.
 */
#ifndef CONNECTION_LISTENER_HXX_
#define CONNECTION_LISTENER_HXX_

#include <string>
#include <vector>

#include "packet_processor.hxx"

/**
 * The ConnectionListener class is a PacketProcessor that
 * detects and responds to incomming connection-openning
 * packets.
 *
 * A "connection-openning" packet is a packet meeting the
 * definition of a standard communication packet whose
 * sequence and channel numbers are both zero, followed
 * by a SCG type of STX.
 *
 * If the network protocol hash and application name match what is expected,
 * an abstract function is called to possibly open the connection. Otherwise,
 * an EOT with an appropriate error message is sent.
 */
class ConnectionListener: public PacketProcessor {
public:
  virtual void process(const Antenna::endpoint& source,
                       Antenna* antenna, Tuner* tuner,
                       const byte* data, unsigned len) noth;

protected:
  /**
   * Contains the auxilliary data contained in the most recent STX received.
   */
  std::vector<byte> auxData;

  /**
   * Constructs the ConnectionListener, attaching it as a trigger
   * to the given Tuner.
   */
  ConnectionListener(Tuner*);

  /**
   * Called when a valid, compatible connection request is received.
   * @param source the endpoint representing the remote peer
   * @param antenna the antenna performing the communications
   * @param tuner the tuner used to filter the communications
   * @param errmsg if an error occurs, use this error message, which
   * is an English description of the error; initialised to
   * "Connection Refused".
   * @param errl10n if an error occurs, use this l10n entry for the error;
   * initialised to "connection_refused"
   * @return true if the connection is accepted; false if an error occurs.
   * Note that on returning true, this class takes no further action ---
   * it is up to the subclass to associate the endpoint with a connection, etc.
   */
  virtual bool acceptConnection(const Antenna::endpoint& source,
                                Antenna* antenna, Tuner* tuner,
                                std::string& errmsg,
                                std::string& errl10n) noth = 0;
};

#endif /* CONNECTION_LISTENER_HXX_ */
