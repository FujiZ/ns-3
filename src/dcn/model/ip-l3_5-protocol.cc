#include "ns3/log.h"

#include "ip-l3_5-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IpL3_5Protocol");

NS_OBJECT_ENSURE_REGISTERED (IpL3_5Protocol);

TypeId
IpL3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IpL3_5Protocol")
    .SetParent<IpL4Protocol> ()
    .SetGroupName ("DCN")
  ;
  return tid;
}

IpL3_5Protocol::~IpL3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

} //namespace ns3
