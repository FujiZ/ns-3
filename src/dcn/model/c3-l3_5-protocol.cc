#include "c3-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

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
C3L3_5Protocol::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination,
                      uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol);
  /*
  NS_ASSERT (source == route->GetSource ());

  C3Header c3Header;
  c3Header.SetNextHeader (protocol);
  /// \todo GetEcnHandler (dest).Send (packet, c3Header, protocol);
  packet->AddHeader (c3Header);
  /// \todo ecn handler should be placed here
  /// 根据不同的L4协议，来判断是否要在Header中标记ACK和ECE
  /// incoming congestion detection : 根据ip包中的ECN标记更新对应incoming链路的拥塞信息
  C3Tag c3Tag;
  uint32_t packetSize = GetPacketSize (packet, protocol);
  // check tag before send
  if (packet->RemovePacketTag (c3Tag))
    {
      // set packet size before forward down
      c3Tag.SetPacketSize (packetSize);
      packet->AddPacketTag (c3Tag);
      auto it = m_divisionMap.find (destination);
      if (it == m_divisionMap.end ())
        {
          Ptr<C3Division> division = CreateObject<C3Division> (source, destination);
          division->SetRoute (route);
          division->SetDownTarget (MakeCallback (&C3L3_5Protocol::ForwardDown, this));
          m_divisionMap[destination] = division;
          it = m_divisionMap.find (destination);
        }
      it->second->Send (packet);
    }
  else
    {
      // packet not contain data. eg: ACK packet
    }
  */
  NS_LOG_DEBUG ("in c3p before forward down");
  ForwardDown (packet, source, destination, protocol, route);
}

void
C3L3_5Protocol::Send6 (Ptr<Packet> packet, Ipv6Address source, Ipv6Address destination,
                       uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << source << destination << (uint32_t)protocol << route);

  Ptr<Packet> copy = packet->Copy ();
  ForwardDown6 (copy, source, destination, protocol, route);
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);

  /// \todo implementation of c3p
  return ForwardUp (packet, header, incomingInterface, header.GetProtocol ());
}

enum IpL4Protocol::RxStatus
C3L3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress ());
  ///\todo implementation of c3p
  return ForwardUp6 (packet, header, incomingInterface, header.GetNextHeader ());
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
