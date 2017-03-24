#include "addcn-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

//#include "addcn-slice.h"
#include "addcn-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADCNL3_5Protocol");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADCNL3_5Protocol);

TypeId
ADCNL3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADCNL3_5Protocol")
    .SetParent<IpL3_5Protocol> ()
    .SetGroupName ("DCN")
    .AddConstructor<ADCNL3_5Protocol> ()
  ;
  return tid;
}

ADCNL3_5Protocol::ADCNL3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

ADCNL3_5Protocol::~ADCNL3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

void
ADCNL3_5Protocol::Send (Ptr<Packet> packet,
                      Ipv4Address src, Ipv4Address dst,
                      uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << (uint32_t)protocol);
  NS_ASSERT (src == route->GetSource ());
  NS_LOG_DEBUG ("in c3p before forward down");

  C3Tag c3Tag;
  TcpHeader tcpHeader;

  // check tag before send
  if(packet->RemovePacketTag (c3Tag))
  {
    NS_ASSERT (packet->RemoveHeader(tcpHeader));
    // C3_Flow cFlow = ADCNDivision::GetDivision(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(src, dst, srcPort, dstPort, protocol);
  }
  {
      if(tcpHeader.GetFlags () & (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR) == (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR))
      {
         // DoConnect step 0, with ECN capability;
      }
      else if(tcpHeader.GetFlags () & (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR) == TcpHeader::SYN)
      {
         // DoConnect step 0, without ECN capability;
         // Record
         tcpHeader |= (TcpHeader::ECE | TcpHeader::CWR); // Enable by force
      }
      packet->AddHeader(header);

  }
  {
      // set packet size before forward down
      c3Tag.SetPacketSize (GetPacketSize (packet, protocol));
      packet->AddPacketTag (c3Tag);
      Ptr<ADCNTunnel> tunnel = ADCNDivision::GetDivision (c3Tag.GetTenantId (), c3Tag.GetType ())->GetTunnel (src, dst);
      tunnel->SetForwardTarget (MakeCallback (&ADCNL3_5Protocol::ForwardDown, this));
      tunnel->SetRoute (route);
      tunnel->Send (packet, protocol);
  }
  {
      // not a data packet: ACK or sth else
      ForwardDown (packet, src, dst, protocol, route);
  }
}

void
ADCNL3_5Protocol::Send6 (Ptr<Packet> packet,
                       Ipv6Address src, Ipv6Address dst,
                       uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << (uint32_t)protocol << route);

  Ptr<Packet> copy = packet->Copy ();
  ForwardDown6 (copy, src, dst, protocol, route);
}

enum IpL4Protocol::RxStatus
ADCNL3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);

  ADCNTag c3Tag;
  if (packet->PeekPacketTag (c3Tag))
    {
      ADCNEcnRecorder::GetEcnRecorder (c3Tag.GetTenantId (), c3Tag.GetType (),
                                     header.GetSource (), header.GetDestination ())->NotifyReceived (header);
    }
  return ForwardUp (packet, header, incomingInterface, header.GetProtocol ());
}

enum IpL4Protocol::RxStatus
ADCNL3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress ());
  return ForwardUp6 (packet, header, incomingInterface, header.GetNextHeader ());
}

void
ADCNL3_5Protocol::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  IpL3_5Protocol::DoDispose ();
}

uint32_t
ADCNL3_5Protocol::GetPacketSize (Ptr<Packet> packet, uint8_t protocol)
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
