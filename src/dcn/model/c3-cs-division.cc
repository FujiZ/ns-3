#include "c3-cs-division.h"

#include "ns3/log.h"

#include "c3-cs-tunnel.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3CsDivision");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3CsDivision);

TypeId
C3CsDivision::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3CsDivision")
      .SetParent<C3Division> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3CsDivision> ();
  ;
  return tid;
}

C3CsDivision::C3CsDivision ()
  : C3Division (C3Type::CS)
{
  NS_LOG_FUNCTION (this);
}

C3CsDivision::~C3CsDivision ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<C3Tunnel>
C3CsDivision::GetTunnel (const Ipv4Address &src, const Ipv4Address &dst)
{
  NS_LOG_FUNCTION (this << src << dst);
  auto it = m_tunnelList.find (std::make_pair (src, dst));
  if (it != m_tunnelList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3CsTunnel> tunnel = CreateObject<C3CsTunnel> (GetTenantId (), src, dst);
      m_tunnelList[std::make_pair (src, dst)] = tunnel;
      return tunnel;
    }
}

} //namespace dcn
} //namespace ns3
