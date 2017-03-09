#ifndef C3_FLOW_H
#define C3_FLOW_H

#include <stdint.h>

#include "ns3/packet.h"

#include "rate-controller.h"
#include "token-bucket-filter.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 flow
 * the base class for various type of flow (eg: LS, DS)
 */
class C3Flow : public RateController
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Flow ();

  virtual ~C3Flow ();
  /**
   * \brief callback to forward packets
   */
  typedef TokenBucketFilter::SendTargetCallback ForwardTargetCallback;
  /**
   * \brief set forward target
   * \param cb forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);
  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for controller to receive
   * make sure that the packet contain c3tag before pass in it
   * the packet size should be marked in c3l3.5p
   */
  virtual void Send (Ptr<Packet> packet);

protected:
  virtual void DoDispose (void);
  /**
   * @brief Forward callback to forward packets
   * @param packet the packet tobe sent out
   * usually used as callback
   */
  virtual void Forward (Ptr<Packet> packet);
  /**
   * @brief Drop callback to drop packet
   * @param packet the packet to drop
   * used as callback
   */
  virtual void Drop (Ptr<const Packet> packet);

protected:
  Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  ForwardTargetCallback m_forwardTarget;    //!< callback to forward packet

  int32_t m_flowSize;    //!< the total size of current flow
  int32_t m_sendedSize;  //!< the sended size of current flow
  int32_t m_bufferSize;  //!< the size of current buffer
  ///\todo add counter to count the send byte
  /// in order to decide when to dispose the flow
  /// separate send and doSend ?
};

} //namespace dcn
} //namespace ns3

#endif // C3_FLOW_H
