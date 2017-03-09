#include "c3-tunnel.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Tunnel");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Tunnel);

TypeId
C3Tunnel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Tunnel")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3Tunnel::C3Tunnel (const Ipv4Address &src, const Ipv4Address &dst)
  : m_weightRequest (0.0),
    m_weightResponse (0.0),
    m_src (src),
    m_dst (dst)
{
  NS_LOG_FUNCTION (this);
}

C3Tunnel::~C3Tunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Tunnel::SetRoute (Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << route);
  m_route = route;
}

void
C3Tunnel::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

double
C3Tunnel::GetWeightRequest (void) const
{
  return m_weightRequest;
}

void
C3Tunnel::SetWeightResponse (double weight)
{
  NS_LOG_FUNCTION (this << weight);
  m_weightResponse = weight;
}

void
C3Tunnel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget.Nullify ();
  m_route = 0;
}

void
C3Tunnel::Forward (Ptr<Packet> packet,  uint8_t protocol)
{
  NS_LOG_FUNCTION (this << packet << (uint32_t) protocol);
  m_forwardTarget (packet, m_src, m_dst, protocol, m_route);
}

} //namespace dcn
} //namespace ns3
