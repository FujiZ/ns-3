#include "ip-l3_5-protocol-helper.h"

#include "ns3/callback.h"
#include "ns3/ip-l3_5-protocol.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IpL3_5ProtocolHelper");

namespace dcn {

IpL3_5ProtocolHelper::IpL3_5ProtocolHelper()
{
  NS_LOG_FUNCTION (this);
}

IpL3_5ProtocolHelper::IpL3_5ProtocolHelper(const std::string &tid)
  : m_agentFactory (tid)
{
  NS_LOG_FUNCTION (this);
}

IpL3_5ProtocolHelper::~IpL3_5ProtocolHelper ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<IpL3_5Protocol>
IpL3_5ProtocolHelper::Create () const
{
  NS_LOG_FUNCTION (this);
  Ptr<IpL3_5Protocol> agent = m_agentFactory.Create<IpL3_5Protocol> ();
  return agent;
}

void
IpL3_5ProtocolHelper::Install (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);
  Ptr<IpL3_5Protocol> agent = Create ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  // deal with the downtargets
  for (L4ListValue_t l4ProtoInfo : m_ipL4Protocols)
    {
      Ptr<IpL4Protocol> l4Protocol = node->GetObject<IpL4Protocol> (l4ProtoInfo.first);
      NS_ASSERT (l4Protocol);
      agent->SetProtocolNumber (l4Protocol->GetProtocolNumber ());
      agent->SetDownTarget (l4Protocol->GetDownTarget ());
      agent->SetDownTarget6 (l4Protocol->GetDownTarget6 ());
      l4Protocol->SetDownTarget (MakeCallback (&IpL3_5Protocol::Send, agent));
      l4Protocol->SetDownTarget6 (MakeCallback (&IpL3_5Protocol::Send6, agent));
      if (l4ProtoInfo.second >=0)
        {
          if (ipv4 != 0)
            {
              ipv4->Remove (l4Protocol, l4ProtoInfo.second);
              ipv4->Insert (agent, l4ProtoInfo.second);
            }
          if (ipv6 != 0)
            {
              ipv6->Remove (l4Protocol, l4ProtoInfo.second);
              ipv6->Insert (agent, l4ProtoInfo.second);
            }
          agent->Insert (l4Protocol, l4ProtoInfo.second);
        }
      else
        {
          if (ipv4 != 0)
            {
              ipv4->Remove (l4Protocol);
              ipv4->Insert (agent);
            }
          if (ipv6 != 0)
            {
              ipv6->Remove (l4Protocol);
              ipv6->Insert (agent);
            }
          agent->Insert (l4Protocol);
        }
    }
  agent->SetNode (node);
  node->AggregateObject (agent);
}

void
IpL3_5ProtocolHelper::Install (NodeContainer nodes) const
{
  for (NodeContainer::Iterator i = nodes.Begin ();
       i != nodes.End (); ++i)
    {
      Install (*i);
    }
}

void
IpL3_5ProtocolHelper::SetAttribute (const std::string &name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

void
IpL3_5ProtocolHelper::SetIpL3_5Protocol (const std::string &tid)
{
  m_agentFactory.SetTypeId (tid);
}

void
IpL3_5ProtocolHelper::AddIpL4Protocol (const std::string &tid)
{
  L4ListValue_t protocol = std::make_pair (TypeId::LookupByName (tid), -1);
  m_ipL4Protocols.push_back (protocol);
}

void
IpL3_5ProtocolHelper::AddIpL4Protocol (const std::string &tid, uint32_t interface)
{
  L4ListValue_t protocol = std::make_pair (TypeId::LookupByName (tid), interface);
  m_ipL4Protocols.push_back (protocol);
}

} //namespace dcn
} //namespace ns3
