#ifndef C3_DIVISION_H
#define C3_DIVISION_H

#include <map>

#include "ns3/callback.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-route.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include "c3-ds-tunnel.h"
#include "rate-controller.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 division implementation
 *
 */
class C3Division : public RateController
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Division (const Ipv4Address& src, const Ipv4Address& dst);

  virtual ~C3Division ();

  //inherited from rate controller
  virtual uint64_t UpdateRateRequest (void);
  virtual void SetRateResponse (uint64_t rate);

  /**
   * \brief set the route of connection
   * \param route new route info
   */
  void SetRoute (Ptr<Ipv4Route> route);

  /**
   * \brief callback to forward packets down to dest
   */
  typedef Callback<void, Ptr<Packet>, Ipv4Address, Ipv4Address, Ptr<Ipv4Route> > DownTargetCallback;

  /**
   * \brief set down target
   */
  void SetDownTarget (DownTargetCallback cb);

  /**
   * @brief Send
   * @param p the packet tobe send
   * add a packet to queue
   */
  void Send (Ptr<Packet> packet);

  /**
   * \brief callback to forward packets to upper layer
   */
  typedef Callback<enum IpL4Protocol::RxStatus, Ptr<Packet>, Ipv4Header const &, Ptr<Ipv4Interface> > UpTargetCallback;

  /**
   * \brief set up target
   */
  void SetUpTarget (UpTargetCallback cb);


  enum IpL4Protocol::RxStatus Receive (Ptr<Packet> packet,
                                       Ipv4Header const &header,
                                       Ptr<Ipv4Interface> incomingInterface);
  /**
   * \todo callback to send packets back to src
   * backwardTargetCallback
   */
protected:

  virtual void DoDispose (void);
  /**
   * \brief forward a packet down to dest
   * \param packet packet to be forward
   * usually used as callback
   */
  void ForwardDown (Ptr<Packet> packet);

private:
  Ipv4Address m_source;   //!< source address of division
  Ipv4Address m_destination;  //!< dst address of division
  Ptr<Ipv4Route> m_route; //!< route of connection
  DownTargetCallback m_downTarget;  //!< down target
  UpTargetCallback m_upTarget;  //!< up target
  /// should be placed elsewhere(C3EcnHandler?)
  //ecn stats
  uint32_t m_totalAck;
  uint32_t m_markedAck;
  double m_alpha;
  double m_g;
  ///\todo incoming congestion status
  ///uint32_t m_incomingPackets;
  ///uint32_t m_incomingECNMarkedPackets;
  ///uint32_t m_incomingAlpha;
  ///uint32_t m_incomingG;

  ///\todo tunnel map|set
  Ptr<C3DsTunnel> m_tunnel;     //!< \todo just one tunnel for test

  //std::map<C3Tag::Type, Ptr<C3Tunnel> > tunnelMap;
  //c3tag, c3dstag, c3cstag etc.
};

} //namespace dcn
} //namespace ns3

#endif // C3_DIVISION_H
