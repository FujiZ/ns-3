#include "ip-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"

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

IpL3_5Protocol::IpL3_5Protocol ()
  : m_protocolNumber (0)
{
  NS_LOG_FUNCTION (this);
}

IpL3_5Protocol::~IpL3_5Protocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
IpL3_5Protocol::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
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

int
IpL3_5Protocol::GetProtocolNumber (void) const
{
  return m_protocolNumber;
}

void
IpL3_5Protocol::SetProtocolNumber (uint8_t protocolNumber)
{
  NS_LOG_FUNCTION (this << (int)protocolNumber);
  m_protocolNumber = protocolNumber;
}

void
IpL3_5Protocol::Insert (Ptr<IpL4Protocol> protocol)
{
  NS_LOG_FUNCTION (this << protocol);
  Insert (protocol, -1);
}

void
IpL3_5Protocol::Insert (Ptr<IpL4Protocol> protocol, int32_t interfaceIndex)
{
  NS_LOG_FUNCTION (this << protocol << interfaceIndex);

  L4ListKey_t key = std::make_pair (protocol->GetProtocolNumber (), interfaceIndex);
  if (m_protocols.find (key) != m_protocols.end ())
    {
      NS_LOG_WARN ("Overwriting protocol " << int(protocol->GetProtocolNumber ()) << " on interface " << int(interfaceIndex));
    }
  m_protocols[key] = protocol;
}

void
IpL3_5Protocol::Remove (Ptr<IpL4Protocol> protocol)
{
  NS_LOG_FUNCTION (this << protocol);

  Remove (protocol, -1);
}

void
IpL3_5Protocol::Remove (Ptr<IpL4Protocol> protocol, int32_t interfaceIndex)
{
  NS_LOG_FUNCTION (this << protocol << interfaceIndex);

  L4ListKey_t key = std::make_pair (protocol->GetProtocolNumber (), interfaceIndex);
  L4List_t::iterator iter = m_protocols.find (key);
  if (iter == m_protocols.end ())
    {
      NS_LOG_WARN ("Trying to remove an non-existent protocol " << int(protocol->GetProtocolNumber ()) << " on interface " << int(interfaceIndex));
    }
  else
    {
      m_protocols.erase (key);
    }
}

Ptr<IpL4Protocol>
IpL3_5Protocol::GetProtocol (int protocolNumber) const
{
  NS_LOG_FUNCTION (this << protocolNumber);

  return GetProtocol (protocolNumber, -1);
}

Ptr<IpL4Protocol>
IpL3_5Protocol::GetProtocol (int protocolNumber, int32_t interfaceIndex) const
{
  NS_LOG_FUNCTION (this << protocolNumber << interfaceIndex);

  L4ListKey_t key;
  L4List_t::const_iterator i;
  if (interfaceIndex >= 0)
    {
      // try the interface-specific protocol.
      key = std::make_pair (protocolNumber, interfaceIndex);
      i = m_protocols.find (key);
      if (i != m_protocols.end ())
        {
          return i->second;
        }
    }
  // try the generic protocol.
  key = std::make_pair (protocolNumber, -1);
  i = m_protocols.find (key);
  if (i != m_protocols.end ())
    {
      return i->second;
    }

  return 0;
}

void
IpL3_5Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_downTarget6.Nullify ();
  m_downTarget.Nullify ();
  m_node = 0;
  m_protocols.clear ();
  m_protocolNumber = 0;
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
  NS_LOG_FUNCTION (this << p << header << incomingInterface << (int)protocolNumber);

  Ptr<Packet> copy = p->Copy ();
  Ptr<Ipv4L3Protocol> l3Proto = m_node->GetObject<Ipv4L3Protocol> ();
  NS_ASSERT_MSG (l3Proto != 0, "Can't get L3Protocol");
  int32_t interface = l3Proto->GetInterfaceForDevice (incomingInterface->GetDevice ());
  Ptr<IpL4Protocol> protocol = GetProtocol (protocolNumber, interface);
  NS_ASSERT_MSG (protocol != 0, "Can't get L4Protocol");
  // we need to make a copy in the unlikely event we hit the
  // RX_ENDPOINT_UNREACH code path
  enum IpL4Protocol::RxStatus status =
    protocol->Receive (copy, header, incomingInterface);
  NS_LOG_DEBUG ("The receive status " << status);
  return status;
}

IpL4Protocol::RxStatus
IpL3_5Protocol::ForwardUp6 (Ptr<Packet> p,
                           const Ipv6Header &header,
                           Ptr<Ipv6Interface> incomingInterface,
                           uint8_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << header << incomingInterface << (int)protocolNumber);

  Ptr<Packet> copy = p->Copy ();

  Ptr<Ipv6L3Protocol> l3Proto = m_node->GetObject<Ipv6L3Protocol> ();
  NS_ASSERT_MSG (l3Proto != 0, "Can't get L3Protocol");
  int32_t interface = l3Proto->GetInterfaceForDevice (incomingInterface->GetDevice ());
  Ptr<IpL4Protocol> protocol = GetProtocol (protocolNumber, interface);
  NS_ASSERT_MSG (protocol != 0, "Can't get L4Protocol");
  // we need to make a copy in the unlikely event we hit the
  // RX_ENDPOINT_UNREACH code path
  enum IpL4Protocol::RxStatus status =
    protocol->Receive (copy, header, incomingInterface);
  NS_LOG_DEBUG ("The receive status " << status);
  return status;
}


void
IpL3_5Protocol::ForwardDown (Ptr<Packet> p,
                             Ipv4Address source, Ipv4Address destination,
                             uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << p << source << destination << route);
  NS_ASSERT_MSG (!m_downTarget.IsNull (), "Error, IpL3.5Protocol cannot send downward");
  m_downTarget (p, source, destination, protocol, route);
}

void
IpL3_5Protocol::ForwardDown6 (Ptr<Packet> p,
                              Ipv6Address source, Ipv6Address destination,
                              uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << p << source << destination << route);
  NS_ASSERT_MSG (!m_downTarget.IsNull (), "Error, IpL3.5Protocol cannot send downward");
  m_downTarget6 (p, source, destination, protocol, route);
}

} //namespace dcn
} //namespace ns3
