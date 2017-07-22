#include "c3-ls-division.h"

#include "ns3/log.h"

#include "c3-ls-tunnel.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3LsDivision");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3LsDivision);

TypeId
C3LsDivision::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3LsDivision")
      .SetParent<C3Division> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3LsDivision> ();
  ;
  return tid;
}

C3LsDivision::C3LsDivision ()
  : C3Division (C3Type::LS)
{
  NS_LOG_FUNCTION (this);
}

C3LsDivision::~C3LsDivision ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<C3Tunnel>
C3LsDivision::GetTunnel (const Ipv4Address &src, const Ipv4Address &dst)
{
  NS_LOG_FUNCTION (this << src << dst);
  auto it = m_tunnelList.find (std::make_pair (src, dst));
  if (it != m_tunnelList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3LsTunnel> tunnel = CreateObject<C3LsTunnel> (GetTenantId (), src, dst);
      m_tunnelList[std::make_pair (src, dst)] = tunnel;
      return tunnel;
    }
}

} //namespace dcn
} //namespace ns3
