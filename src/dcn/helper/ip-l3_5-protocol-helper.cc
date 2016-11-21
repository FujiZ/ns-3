#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/callback.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/packet.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"

#include "ns3/ip-l3_5-protocol.h"

#include "ip-l3_5-protocol-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IpL3_5ProtocolHelper");

IpL3_5ProtocolHelper::IpL3_5ProtocolHelper()
{
  NS_LOG_FUNCTION (this);
}

IpL3_5ProtocolHelper::IpL3_5ProtocolHelper(std::string tid)
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
  // deal with the downtargets
  for (TypeId ipL4ProtoId : m_ipL4Protocols)
    {
      Ptr<IpL4Protocol> l4Protocol = node->GetObject<IpL4Protocol> (ipL4ProtoId);
      agent->SetDownTarget (l4Protocol->GetDownTarget ());
      l4Protocol->SetDownTarget (MakeCallback (&IpL3_5Protocol::Send, agent));
      agent->SetDownTarget6 (l4Protocol->GetDownTarget6 ());
      l4Protocol->SetDownTarget6 (MakeCallback (&IpL3_5Protocol::Send6, agent));
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
IpL3_5ProtocolHelper::SetIpL3_5Protocol (TypeId tid)
{
  m_agentFactory.SetTypeId (tid);
}

void
IpL3_5ProtocolHelper::SetIpL3_5Protocol (std::string tid)
{
  SetIpL3_5Protocol (TypeId::LookupByName (tid));
}

void
IpL3_5ProtocolHelper::AddIpL4Protocol (TypeId tid)
{
  m_ipL4Protocols.push_back (tid);
}

void
IpL3_5ProtocolHelper::AddIpL4Protocol (std::string tid)
{
  AddIpL4Protocol (TypeId::LookupByName (tid));
}

}
