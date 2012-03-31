/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.05
 * @brief Defines the SynchronousControlGeraet network input-output Gerät.
 */
#ifndef SYNCHRONOUS_CONTROL_GERAET_HXX_
#define SYNCHRONOUS_CONTROL_GERAET_HXX_

#include <deque>
#include <utility>

#include "network_geraet.hxx"
#include "network_connection.hxx"

/**
 * Controls the basics of the network protocol.
 *
 * Each NetworkConnection has exactly one of these, which is both an input
 * and output Gerät, and is opened and bound to channel 0 at creation time.
 */
class SynchronousControlGeraet:
public InputNetworkGeraet,
public OutputNetworkGeraet {
  friend class NetworkConnection;

  //Data that must be sent in XONs
  struct XonDat {
    NetworkConnection::channel chan;
    NetworkConnection::geraet_num remoteGeraet;
    OutputNetworkGeraet* localGeraet;
  };
  std::deque<XonDat> xonsOut;
  //The number of elements at the front of xonsOut that were sent
  //in the most recent unacknowledged XON
  unsigned lastXonLen;
  //Data that must be sent in XOFs
  std::deque<NetworkConnection::channel> xofsOut;
  //The number of elements at the front of xofsOut that were sent
  //in the most recent unacknowledged XOF
  unsigned lastXofLen;

  //The time since the last packet transmission
  unsigned timeSinceTxn;

  //The sequence number of the most recently transmitted ACK-requiring packet
  NetworkConnection::seq_t lastPackOutSeq;
  //The type of the most recently transmitted unacknowledged ACK-requiring
  //packet, or 0 if there is none.
  byte lastPackOutType;

  //Currently unused channel numbers
  std::deque<NetworkConnection::channel> freeChannels;

  //Auxilliary data received in STX
  std::vector<byte> auxData;
  //Auxilliary data to send in STX
  std::vector<byte> auxDataOut;

  /**
   * Constructs a new SynchronousControlGeraet for the given connection.
   * @param cxn the connection to operate on
   * @param incomming whether this is an incomming connection or
   * outgoing
   */
  SynchronousControlGeraet(NetworkConnection* cxn, bool incomming);

public:
  virtual ~SynchronousControlGeraet();

  /**
   * Opens an outgoing channel.
   *
   * @param geraet the output Gerät to use with the channel; the setChannel()
   * function will be called to tell the Gerät of its new channel number.
   * @param number the Gerät number for the remote peer to use
   * @returns the number of the channel that will be opened
   */
  NetworkConnection::channel
  openChannel(OutputNetworkGeraet* geraet,
              NetworkConnection::geraet_num number) noth;


  /**
   * Closes the given outgoing channel.
   */
  void closeChannel(NetworkConnection::channel) noth;
  /**
   * Closes the outgoing connection with the given reason.
   *
   * @param english the English explanation of termination
   * @param l10n the l10n entry for the explanation
   */
  void closeConnection(const char* english = "Connection closed.",
                       const char* l10n = "connection_closed") noth;

  virtual void receive(NetworkConnection::seq_t seq,
                       const byte* data, unsigned len)
  throw();

  /**
   * Returns the auxilliary data contained in the STX that transitioned the
   * connection from Connecting to Established.
   *
   * The vector will be empty if no such packet has been received; this is the
   * case when the connection was initiated by a ConnectionListener, or if the
   * connection is still Connecting.
   */
  const std::vector<byte>& getAuxData() const throw() {
    return auxData;
  }

  /**
   * Sets the auxilliary data for outgoing STX packets to the given data.
   */
  template<typename InputIterator>
  void setAuxDataOut(InputIterator begin, InputIterator end) throw() {
    auxDataOut.assign(begin, end);
  }

  virtual void update(unsigned et) throw();

private:
  void transmitStx() throw();
  void transmitXon() throw();
  void transmitXof() throw();
  void transmitAck(NetworkConnection::seq_t) throw();
};

#endif /* SYNCHRONOUS_CONTROL_GERAET_HXX_ */
