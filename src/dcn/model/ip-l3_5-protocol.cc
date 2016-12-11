#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-interface.h"

#include "ip-l3_5-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IpL3_5Protocol");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (IpL3_5Protocol);

TypeId
IpL3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn:IpL3_5Protocol")
    .SetParent<IpL4Protocol> ()
    .SetGroupName ("DCN")
  ;
  return tid;
}

IpL3_5Protocol::~IpL3_5Protocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
IpL3_5Protocol::SetNode (Ptr<Node> node)
{
  m_node = node;
}


void
IpL3_5Protocol::SetDownTarget (IpL4Protocol::DownTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_downTarget = cb;
}

void
IpL3_5Protocol::SetDownTarget6 (IpL4Protocol::DownTargetCallback6 cb)
{
  NS_LOG_FUNCTION (this);
  m_downTarget6 = cb;
}

IpL4Protocol::DownTargetCallback
IpL3_5Protocol::GetDownTarget (void) const
{
  return m_downTarget;
}

IpL4Protocol::DownTargetCallback6
IpL3_5Protocol::GetDownTarget6 (void) const
{
  return m_downTarget6;
}

void
IpL3_5Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = 0;
  m_downTarget.Nullify ();
  m_downTarget6.Nullify ();
  IpL4Protocol::DoDispose ();
}

void
IpL3_5Protocol::NotifyNewAggregate (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<Ipv4> ipv4 = this->GetObject<Ipv4> ();
  Ptr<Ipv6> ipv6 = this->GetObject<Ipv6> ();

  if (m_node == 0)
    {
      if ((node != 0) && (ipv4 != 0 || ipv6 != 0))
        {
          // had internet stack on it
          this->SetNode (node);
        }
    }

  // We set at least one of our 2 down targets to the IPv4/IPv6 send
  // functions.  Since these functions have different prototypes, we
  // need to keep track of whether we are connected to an IPv4 or
  // IPv6 lower layer and call the appropriate one.
  if (ipv4 != 0)
    {
      ipv4->Insert (this);
      if (m_downTarget.IsNull ())
        {
          this->SetDownTarget (MakeCallback (&Ipv4::Send, ipv4));
        }
    }
  if (ipv6 != 0)
    {
      ipv6->Insert (this);
      if (m_downTarget6.IsNull ())
        {
          this->SetDownTarget6 (MakeCallback (&Ipv6::Send, ipv6));
        }
    }
  IpL4Protocol::NotifyNewAggregate ();
}

IpL4Protocol::RxStatus
IpL3_5Protocol::ForwardUp (Ptr<Packet> p,
                           const Ipv4Header &header,
                           Ptr<Ipv4Interface> incomingInterface,
                           uint8_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << header << incomingInterface);
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  Ptr<Packet> copy = p->Copy ();

  Ptr<Ipv4L3Protocol> l3Proto = m_node->GetObject<Ipv4L3Protocol> ();
  Ptr<IpL4Protocol> nextProto = l3Proto->GetProtocol (protocolNumber);
  if(nextProto != 0)
    {
      // we need to make a copy in the unlikely event we hit the
      // RX_ENDPOINT_UNREACH code path
      enum IpL4Protocol::RxStatus status =
        nextProto->Receive (copy, header, incomingInterface);
      NS_LOG_DEBUG ("The receive status " << status);
      return status;
    }
  else
    {
      NS_FATAL_ERROR ("Should not have 0 next protocol value");
    }
}

IpL4Protocol::RxStatus
IpL3_5Protocol::ForwardUp6 (Ptr<Packet> p,
                           const Ipv6Header &header,
                           Ptr<Ipv6Interface> incomingInterface,
                           uint8_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << header << incomingInterface);
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  Ptr<Packet> copy = p->Copy ();

  Ptr<Ipv6L3Protocol> l3Proto = m_node->GetObject<Ipv6L3Protocol> ();
  Ptr<IpL4Protocol> nextProto = l3Proto->GetProtocol (protocolNumber);
  if(nextProto != 0)
    {
      // we need to make a copy in the unlikely event we hit the
      // RX_ENDPOINT_UNREACH code path
      enum IpL4Protocol::RxStatus status =
        nextProto->Receive (copy, header, incomingInterface);
      NS_LOG_DEBUG ("The receive status " << status);
      return status;
    }
  else
    {
      NS_FATAL_ERROR ("Should not have 0 next protocol value");
    }
}

void
IpL3_5Protocol::ForwardDown (Ptr<Packet> p,
                             Ipv4Address source, Ipv4Address destination,
                             Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << p << source << destination << route);
  NS_ASSERT_MSG (!m_downTarget.IsNull (), "Error, IpL3.5Protocol cannot send downward");
  Ptr<Packet> copy = p->Copy ();
  m_downTarget (copy, source, destination, GetProtocolNumber (), route);
}

void
IpL3_5Protocol::ForwardDown6 (Ptr<Packet> p,
                              Ipv6Address source, Ipv6Address destination,
                              Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << p << source << destination << route);
  NS_ASSERT_MSG (!m_downTarget.IsNull (), "Error, IpL3.5Protocol cannot send downward");
  Ptr<Packet> copy = p->Copy ();
  m_downTarget6 (copy, source, destination, GetProtocolNumber (), route);
}

void
IpL3_5Protocol::ForwardDownStatic (IpL3_5Protocol *protocol, Ipv4Address destination,
                                   Ptr<Ipv4Route> route, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (protocol << route->GetSource () << destination << p);
  protocol->ForwardDown (p, route->GetSource (), destination, route);
}

} //namespace dcn
} //namespace ns3
