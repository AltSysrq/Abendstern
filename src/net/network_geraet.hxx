/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.03
 * @brief Contains the NetworkGeraet class
 */
#ifndef NETWORK_GERAET_HXX_
#define NETWORK_GERAET_HXX_

#include "antenna.hxx"
#include "network_connection.hxx"

/**
 * Defines the interface common to all input network Geräte.
 */
class InputNetworkGeraet: public AObject {
  friend class SynchronousControlGeraet;
protected:
  ///The input channel that packets are received through
  NetworkConnection::channel inputChannel;
public:
  /**
   * DeletionStrategies inform NetworkConnection and the
   * SynchronousControlGerät how to manage the memory associated
   * with this input Gerät.
   */
  enum DeletionStrategy {
    /**
     * The InputNetworkGeraet is input only, and may be externally
     * closed, is deleted when closed, and is deleted when the NetworkConnection
     * frees its input Geräte.
     */
    DSNormal,
    /**
     * The InputNetworkGeraet is bidirectional, and cannot be closed after
     * being opened. It is deleted when the NetworkConnection frees its
     * input Geräte.
     */
    DSEternal,
    /**
     * The InputNetworkGeraet is an intrinsic component of its parent
     * NetworkConnection. It cannot be closed once opened, and is not freed
     * along with the other input Geräte.
     */
    DSIntrinsic
  } const deletionStrategy; ///< The DeletionStrategy for this Gerät

  /**
   * Constructs an InputNetworkGeraet with the given DeletionStrategy,
   * which defaults to DSNormal.
   */
  InputNetworkGeraet(DeletionStrategy ds = DSNormal)
  : deletionStrategy(ds)
  { }

  /**
   * Notifies the InputNetworkGeraet that it has received a packet.
   * @param seq the sequence number of the packet
   * @param data the payload of the packet, excluding sequence and channel
   * numbers
   * @param len the length of the payload
   */
  virtual void receive(NetworkConnection::seq_t seq,
                       const byte* data, unsigned len)
  throw() = 0;

  /**
   * Updates the InputNetworkGeraet based on elapsed time.
   *
   * The default does nothing.
   * @param et the time, in milliseconds, since the previous call to update()
   */
  virtual void inputUpdate(unsigned et) throw() { }
};

/**
 * Defines the interface common to all output network Geräte.
 */
class OutputNetworkGeraet: public AObject {
protected:
  /**
   * The output channel to write to.
   */
  NetworkConnection::channel channel;

  /**
   * The NetworkConnection this Gerät runs under.
   */
  NetworkConnection*const cxn;
public:
  /**
   * DeletionStrategies inform the NetworkConnection and the
   * SynchronousControlGeraet how to manage the memory associated with this
   * OutputNetworkGeraet.
   */
  enum DeletionStrategy {
    /**
     * The OutputNetworkGeraet is output only. It will be deleted on closing
     * and when the parent NetworkConnection frees its output Geräte.
     */
    DSNormal,
    /**
     * The OutputNetworkGeraet is also an input Gerät. It will not be deleted
     * when the parent NetworkConnection frees its output Geräte, as this is
     * assumed to happen when the input Geräte are freed.
     */
    DSBidir,
    /**
     * The OutputNetworkGeraet is a fundamental part of the parent
     * NetworkConnection, and is not freed with the other output Geräte.
     */
    DSIntrinsic
  } const deletionStrategy; ///< The DeletionStrategy for this output Gerät

  /**
   * Constructs an OutputNetworkGeraet on the given NetworkConnection and with
   * the given DeletionStrategy, which defaults to DSNormal.
   * The channel number is NOT initialised.
   */
  OutputNetworkGeraet(NetworkConnection* cxn_, DeletionStrategy ds = DSNormal)
  : cxn(cxn_), deletionStrategy(ds)
  { }

  /**
   * Updates the OutputNetworkGeraet based on elapsed time.
   *
   * The default does nothing.
   * @param et the time, in milliseconds, since the previous call to update()
   */
  virtual void update(unsigned et) throw() { }

  /**
   * Sets the channel the Gerät outputs to.
   */
  void setChannel(NetworkConnection::channel chan) throw() {
    channel = chan;
  }

  /**
   * Called by the SynchronousControlGeraet when the channel is confirmed
   * open. Default does nothing.
   */
  virtual void outputOpen() throw() {}
};
#endif /* NETWORK_GERAET_HXX_ */
