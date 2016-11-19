#include "ns3/log.h"
#include "ns3/ipv4-l3-protocol.h"

#include "c3-l3_5-protocol.h"
#include "c3-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3L3_5Protocol");

NS_OBJECT_ENSURE_REGISTERED (C3L3_5Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t C3L3_5Protocol::PROT_NUMBER = 253;

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
  Ptr<Packet> copy = packet->Copy ();

  uint8_t nextHeader = c3Header.GetNextHeader ();
  Ptr<Ipv4L3Protocol> l3proto = m_node->GetObject<Ipv4L3Protocol> ();
  Ptr<IpL4Protocol> nextProto = l3proto->GetProtocol (nextHeader);
  if (nextProto != 0)
    {
      // we need to make a copy in the unlikely event we hit the
      // RX_ENDPOINT_UNREACH code path
      enum IpL4Protocol::RxStatus status =
        nextProto->Receive (copy, header, incomingInterface);
      NS_LOG_DEBUG ("The receive status " << status);
      switch (status)
        {
        case IpL4Protocol::RX_OK:
        case IpL4Protocol::RX_ENDPOINT_CLOSED:
        case IpL4Protocol::RX_CSUM_FAILED:
          break;
        case IpL4Protocol::RX_ENDPOINT_UNREACH:
          if (header.GetDestination ().IsBroadcast () == true
              || ip.GetDestination ().IsMulticast () == true)
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

}
