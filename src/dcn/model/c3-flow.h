#ifndef C3_SESSION_H
#define C3_SESSION_H

#include "ns3/callback.h"
#include "ns3/ipv4-address.h"

#include "rate-controller.h"

namespace ns3 {

class Packet;
class Ipv4Route;

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
  typedef Callback<void, Ptr<Packet> > ForwardTargetCallback;
  /**
   * \brief set forward target
   */
  void SetForwardTargetCallback (ForwardTargetCallback cb);
  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for controller to receive
   */
  virtual void Send (Ptr<Packet> p) = 0;
protected:

  virtual DoDispose (void);

protected:

  //ForwardTargetCallback m_forwardTargetCallback;  //!< forward target, maybe not necessary
  Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  ///\todo add counter to count the send/receive byte
  /// in order to decide when to dispose the flow
  /// separate send and doSend ?
};

} //namespace dcn
} //namespace ns3

#endif // C3_SESSION_H
