#include "c3-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include "c3-header.h"
#include "c3-tag.h"

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
  NS_LOG_FUNCTION (this);
}

C3L3_5Protocol::~C3L3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

void
C3L3_5Protocol::Send (Ptr<Packet> packet, Ipv4Address source,
                      Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol);
  NS_ASSERT (source == route->GetSource ());

  C3Header c3Header;
  c3Header.SetNextHeader (protocol);
  /// \todo implement c3p
  C3Tag c3Tag;
  // check tag before send
  if (packet->RemovePacketTag (c3Tag))
    {
      // set packet size before forward down
      c3Tag.SetPacketSize (GetPacketSize (packet, protocol));
      packet->AddPacketTag (c3Tag);
      auto it = m_divisionMap.find (destination);
      if (it == m_divisionMap.end ())
        {
          Ptr<C3Division> division = CreateObject<C3Division> (source, destination);
          division->SetRoute (route);
          division->SetForwardTarget (MakeCallback (&C3L3_5Protocol::ForwardDown, this));
          m_divisionMap[destination] = division;
          it = m_divisionMap.find (destination);
        }
      packet->AddHeader (c3Header);
      it->second->Send (packet);
    }
  else
    {
      // packet not contain data. eg: ACK packet
      packet->AddHeader (c3Header);
      ForwardDown (packet, source, destination, route);
    }
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
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  C3Header c3Header;
  packet->RemoveHeader (c3Header);          // Remove the c3 header in whole
  uint8_t nextHeader = c3Header.GetNextHeader ();

  /// \todo implementation of c3p
  return ForwardUp (packet, header, incomingInterface, nextHeader);
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress ());
  /*
   * When forwarding or local deliver packets, this one should be used always!!
   */
  C3Header c3Header;
  packet->RemoveHeader (c3Header);          // Remove the c3 header in whole

  uint8_t nextHeader = c3Header.GetNextHeader ();

  ///\todo implementation of c3p
  return ForwardUp6 (packet, header, incomingInterface, nextHeader);
}

void
C3L3_5Protocol::DoInitialize ()
{
  IpL3_5Protocol::DoInitialize ();
}

void
C3L3_5Protocol::DoDispose ()
{
  ///\todo dispose
  IpL3_5Protocol::DoDispose ();
}

uint32_t
C3L3_5Protocol::GetPacketSize (Ptr<Packet> packet, uint8_t protocol)
{
  /// \todo the calculation of packet size can be placed here
  uint32_t size;
  switch (protocol) {
    case 6: //Tcp
      {
        TcpHeader tcpHeader;
        packet->PeekHeader (tcpHeader);
        size = packet->GetSize () - tcpHeader.GetSerializedSize ();
        break;
      }
    case 17:    //udp
      {
        UdpHeader udpHeader;
        packet->PeekHeader (udpHeader);
        size = packet->GetSize () - udpHeader.GetSerializedSize ();
        break;
      }
    default:
      {
        NS_ABORT_MSG ("Protocol " << (uint32_t)protocol << "not implemented");
        break;
      }
    }
  return size;

}

} //namespace dcn
} //namespace ns3
