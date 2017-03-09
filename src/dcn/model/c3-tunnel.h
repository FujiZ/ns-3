#ifndef C3_TUNNEL_H
#define C3_TUNNEL_H

#include <map>
#include <stdint.h>

#include "ns3/ptr.h"
#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"

#include "rate-controller.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the base class for various type of tunnel (eg: LS, DS)
 */
class C3Tunnel : public RateController
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Tunnel (const Ipv4Address &src, const Ipv4Address &dst);

  virtual ~C3Tunnel ();

  /**
   * \brief set the route of connection
   * \param route new route info
   */
  void SetRoute (Ptr<Ipv4Route> route);

  /**
   * \brief callback to forward packets to dest
   */
  typedef Callback<void, Ptr<Packet>, Ipv4Address, Ipv4Address, Ptr<Ipv4Route> > ForwardTargetCallback;

  /**
   * \brief set forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);

  virtual void Send (Ptr<Packet> packet) = 0;

protected:
  /**
   * \brief forward a packet to dest
   * \param p packet to be forward
   * usually used as callback
   */
  void Forward (Ptr<Packet> p);

private:
  Ipv4Address m_source;   //!< source address of tunnel
  Ipv4Address m_destination;  //!< dst address of tunnel
  Ptr<Ipv4Route> m_route; //!< route of connection
  ForwardTargetCallback m_forwardTarget;  //!< forward target
};

} //namespace dcn
} //namespace ns3

#endif // C3_TUNNEL_H
