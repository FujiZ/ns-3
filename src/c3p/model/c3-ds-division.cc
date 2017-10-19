#include "c3-ds-division.h"

#include "ns3/log.h"

#include "c3-ds-tunnel.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3DsDivision");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3DsDivision);

TypeId
C3DsDivision::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3DsDivision")
      .SetParent<C3Division> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3DsDivision> ();
  ;
  return tid;
}

C3DsDivision::C3DsDivision ()
  : C3Division (C3Type::DS)
{
  NS_LOG_FUNCTION (this);
}

C3DsDivision::~C3DsDivision ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<C3Tunnel>
C3DsDivision::GetTunnel (const Ipv4Address &src, const Ipv4Address &dst)
{
  NS_LOG_FUNCTION (this << src << dst);
  auto it = m_tunnelList.find (std::make_pair (src, dst));
  if (it != m_tunnelList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3DsTunnel> tunnel = CreateObject<C3DsTunnel> (GetTenantId (), src, dst);
      m_tunnelList[std::make_pair (src, dst)] = tunnel;
      return tunnel;
    }
}

} //namespace dcn
} //namespace ns3
