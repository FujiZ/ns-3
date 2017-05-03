#include "c3-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/flow-id-tag.h"

#include "c3-division.h"
#include "c3-ecn-recorder.h"
#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3L3_5Protocol");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3L3_5Protocol);

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
  NS_LOG_FUNCTION (this);
}

C3L3_5Protocol::~C3L3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

void
C3L3_5Protocol::Send (Ptr<Packet> packet,
                      Ipv4Address src, Ipv4Address dst,
                      uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << static_cast<uint32_t> (protocol));
  NS_ASSERT (src == route->GetSource ());
  NS_LOG_DEBUG ("in c3p before forward down");

  C3Tag c3Tag;
  // check tag before send
  if (packet->PeekPacketTag (c3Tag))
    {
      Ptr<C3Tunnel> tunnel = C3Division::GetDivision (c3Tag.GetTenantId (), c3Tag.GetType ())->GetTunnel (src, dst);
      tunnel->SetForwardTarget (MakeCallback (&C3L3_5Protocol::ForwardDown, this));
      tunnel->SetRoute (route);
      tunnel->Send (packet, protocol);
    }
  else
    {
      // not a data packet: ACK or sth else
      ///\todo check if we can free tunnel
      ForwardDown (packet, src, dst, protocol, route);
    }
}

void
C3L3_5Protocol::Send6 (Ptr<Packet> packet,
                       Ipv6Address src, Ipv6Address dst,
                       uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << static_cast<uint32_t> (protocol) << route);

  Ptr<Packet> copy = packet->Copy ();
  ForwardDown6 (copy, src, dst, protocol, route);
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);

  C3Tag c3Tag;
  if (packet->PeekPacketTag (c3Tag))
    {
      C3EcnRecorder::GetEcnRecorder (c3Tag.GetTenantId (), c3Tag.GetType (),
                                     header.GetSource (), header.GetDestination ())->NotifyReceived (header);
    }
  return ForwardUp (packet, header, incomingInterface, header.GetProtocol ());
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress ());
  return ForwardUp6 (packet, header, incomingInterface, header.GetNextHeader ());
}

void
C3L3_5Protocol::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  IpL3_5Protocol::DoDispose ();
}

} //namespace dcn
} //namespace ns3
