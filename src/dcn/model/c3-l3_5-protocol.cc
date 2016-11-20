#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv4-l3-protocol.h"

#include "c3-l3_5-protocol.h"
#include "c3-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3L3_5Protocol");

NS_OBJECT_ENSURE_REGISTERED (C3L3_5Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t C3L3_5Protocol::PROT_NUMBER = 253;

TypeId
C3L3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::C3L3_5Protocol")
    .SetParent<IpL3_5Protocol> ()
    .SetGroupName ("DCN")
    .AddConstructor<C3L3_5Protocol> ()
  ;
  return tid;
}

C3L3_5Protocol::C3L3_5Protocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

C3L3_5Protocol::~C3L3_5Protocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
C3L3_5Protocol::SetNode (Ptr<Node> node)
{
  m_node = node;
}

void
C3L3_5Protocol::Send (Ptr<Packet> packet, Ipv4Address source,
                      Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol << route);
  NS_ASSERT_MSG (!m_downTarget.IsNull (), "Error, c3p cannot send downward");

  Ptr<Packet> p = packet->Copy ();
  C3Header c3Header;
  c3Header.SetNextHeader (protocol);
  p->AddHeader (c3Header);
  m_downTarget (p, source, destination, GetProtocolNumber (), route);
}

int
C3L3_5Protocol::GetProtocolNumber (void) const
{
  return PROT_NUMBER;
}


enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> p,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << p << header << incomingInterface);
  Ptr<Packet> packet = p->Copy ();            // Save a copy of the received packet
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  C3Header c3Header;
  packet->RemoveHeader (c3Header);          // Remove the c3 header in whole

  uint8_t nextHeader = c3Header.GetNextHeader ();
  Ptr<Ipv4L3Protocol> l3proto = m_node->GetObject<Ipv4L3Protocol> ();
  Ptr<IpL4Protocol> nextProto = l3proto->GetProtocol (nextHeader);
  if (nextProto != 0)
    {
      // we need to make a copy in the unlikely event we hit the
      // RX_ENDPOINT_UNREACH code path
      enum IpL4Protocol::RxStatus status =
        nextProto->Receive (packet, header, incomingInterface);
      NS_LOG_DEBUG ("The receive status " << status);
      switch (status)
        {
        case IpL4Protocol::RX_OK:
        case IpL4Protocol::RX_ENDPOINT_CLOSED:
        case IpL4Protocol::RX_CSUM_FAILED:
          break;
        case IpL4Protocol::RX_ENDPOINT_UNREACH:
          if (header.GetDestination ().IsBroadcast () == true
              || header.GetDestination ().IsMulticast () == true)
            {
              break;       // Do not reply to broadcast or multicast
            }
          // Another case to suppress ICMP is a subnet-directed broadcast
        }
      return status;
    }
  else
    {
      NS_FATAL_ERROR ("Should not have 0 next protocol value");
    }
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> p,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << p << header.GetSourceAddress () << header.GetDestinationAddress () << incomingInterface);
  return IpL4Protocol::RX_ENDPOINT_UNREACH;
}

void
C3L3_5Protocol::SetDownTarget (IpL4Protocol::DownTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_downTarget = cb;
}
void
C3L3_5Protocol::SetDownTarget6 (IpL4Protocol::DownTargetCallback6 cb)
{
  NS_LOG_FUNCTION (this);
  m_downTarget6 = cb;
}

IpL4Protocol::DownTargetCallback
C3L3_5Protocol::GetDownTarget (void) const
{
  return m_downTarget;
}

IpL4Protocol::DownTargetCallback6
C3L3_5Protocol::GetDownTarget6 (void) const
{
  return m_downTarget6;
}

void
C3L3_5Protocol::NotifyNewAggregate (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<Ipv4> ipv4 = this->GetObject<Ipv4> ();
  Ptr<Ipv6> ipv6 = this->GetObject<Ipv6> ();

  if (m_node == 0)
    {
      if ((node != 0) && (ipv4 != 0 || ipv6 != 0))
        {
          this->SetNode (node);
        }
    }

  // We set at least one of our 2 down targets to the IPv4/IPv6 send
  // functions.  Since these functions have different prototypes, we
  // need to keep track of whether we are connected to an IPv4 or
  // IPv6 lower layer and call the appropriate one.

  if (ipv4 != 0 && m_downTarget.IsNull())
    {
      ipv4->Insert (this);
      this->SetDownTarget (MakeCallback (&Ipv4::Send, ipv4));
    }
  if (ipv6 != 0 && m_downTarget6.IsNull())
    {
      ipv6->Insert (this);
      this->SetDownTarget6 (MakeCallback (&Ipv6::Send, ipv6));
    }
  IpL4Protocol::NotifyNewAggregate ();
}

}
