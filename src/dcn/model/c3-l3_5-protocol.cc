#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-interface.h"

#include "c3-l3_5-protocol.h"
#include "c3-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3L3_5Protocol");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3L3_5Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t C3L3_5Protocol::PROT_NUMBER = 253;

TypeId
C3L3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3L3_5Protocol")
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
  NS_LOG_FUNCTION (this);
}

void
C3L3_5Protocol::Send (Ptr<Packet> packet, Ipv4Address source,
                      Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol << route);


  Ptr<Packet> copy = packet->Copy ();
  C3Header c3Header;
  c3Header.SetNextHeader (protocol);
  copy->AddHeader (c3Header);
  NS_ASSERT (destination == route->GetDestination ());
  /// \todo implement c3p
  m_tbf->SetSendTarget (MakeBoundCallback (&C3L3_5Protocol::ForwardDownStatic,
                                           this, destination, route));
  m_tbf->Receive (copy);
  //ForwardDown (copy, source, destination, route);
}

void
C3L3_5Protocol::Send6 (Ptr<Packet> packet, Ipv6Address source,
                      Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol << route);

  Ptr<Packet> copy = packet->Copy ();
  C3Header c3Header;
  c3Header.SetNextHeader (protocol);
  copy->AddHeader (c3Header);
  ForwardDown6 (copy, source, destination, route);
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
  Ptr<Packet> copy = p->Copy ();            // Save a copy of the received packet
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  C3Header c3Header;
  copy->RemoveHeader (c3Header);          // Remove the c3 header in whole

  uint8_t nextHeader = c3Header.GetNextHeader ();

  /// \todo implementation of c3p
  return ForwardUp (copy, header, incomingInterface, nextHeader);

}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> p,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << p << header.GetSourceAddress () << header.GetDestinationAddress () << incomingInterface);
  Ptr<Packet> copy = p->Copy ();            // Save a copy of the received packet
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  C3Header c3Header;
  copy->RemoveHeader (c3Header);          // Remove the c3 header in whole

  uint8_t nextHeader = c3Header.GetNextHeader ();

  ///\todo implementation of c3p
  return ForwardUp6 (copy, header, incomingInterface, nextHeader);
}

void
C3L3_5Protocol::DoInitialize ()
{
  m_tbf = CreateObject<TokenBucketFilter> ();
}

void
C3L3_5Protocol::DoDispose ()
{
  ///\todo dispose
  IpL3_5Protocol::DoDispose ();
}

} //namespace dcn
} //namespace ns3
