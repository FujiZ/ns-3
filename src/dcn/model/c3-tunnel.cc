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
      .SetParent<RateController> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3Tunnel::~C3Tunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Tunnel::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

void
C3Tunnel::Forward (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  m_forwardTarget (p);
}

} //namespace dcn
} //namespace ns3
