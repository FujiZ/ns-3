#ifndef C3_FLOW_H
#define C3_FLOW_H

#include <stdint.h>

#include "ns3/callback.h"
#include "ns3/ipv4-address.h"

#include "rate-controller.h"
#include "token-bucket-filter.h"

namespace ns3 {

class Packet;

namespace dcn {

class TokenBucketFilter;

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
  virtual void Send (Ptr<Packet> p) = 0;

protected:
  virtual void DoDispose (void);

protected:
  Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  //uint32_t m_remainSize;  //!< the remain size of current flow
  //uint32_t m_bufferSize;//!< the size of current buffer
  ///\todo add counter to count the send/receive byte
  /// in order to decide when to dispose the flow
  /// separate send and doSend ?
};

} //namespace dcn
} //namespace ns3

#endif // C3_FLOW_H
