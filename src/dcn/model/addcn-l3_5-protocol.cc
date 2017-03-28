#include "addcn-l3_5-protocol.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include "addcn-slice.h"
#include "c3-tag.h"
#include "addcn-flow.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADDCNL3_5Protocol");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADDCNL3_5Protocol);

TypeId
ADDCNL3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADDCNL3_5Protocol")
    .SetParent<IpL3_5Protocol> ()
    .SetGroupName ("DCN")
    .AddConstructor<ADDCNL3_5Protocol> ()
  ;
  return tid;
}

ADDCNL3_5Protocol::ADDCNL3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

ADDCNL3_5Protocol::~ADDCNL3_5Protocol ()
{
  NS_LOG_FUNCTION (this);
}

void
ADDCNL3_5Protocol::Send (Ptr<Packet> packet,
                      Ipv4Address src, Ipv4Address dst,
                      uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << (uint32_t)protocol);
  NS_ASSERT (src == route->GetSource ());
  NS_LOG_DEBUG ("in c3p before forward down");

  if(protocol == 6) // TCP
  {
    TcpHeader tcpHeader;
    packet->PeekHeader (tcpHeader);
    C3Tag c3Tag;

    if(packet->RemovePacketTag (c3Tag))
    {
      ADDCNFlow::FiveTuple tuple;
      tuple.sourceAddress = src;
      tuple.destinationAddress = dst;
      tuple.protocol = protocol;

      // we rely on the fact that for both TCP and UDP the ports are
      // carried in the first 4 octects.
      // This allows to read the ports even on fragmented packets
      // not carrying a full TCP or UDP header.

      uint8_t data[4];
      packet->CopyData (data, 4);

      uint16_t srcPort = 0;
      srcPort |= data[0];
      srcPort <<= 8;
      srcPort |= data[1];
 
      uint16_t dstPort = 0;
      dstPort |= data[2];
      dstPort <<= 8;
      dstPort |= data[3];
 
      tuple.sourcePort = srcPort;
      tuple.destinationPort = dstPort;

      //TODO handle FIN TCPHeader
      Ptr<ADDCNFlow> flow = ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(tuple);
      if((tcpHeader.GetFlags() & (TcpHeader::SYN | TcpHeader::ACK)) == TcpHeader::SYN)
      {
        flow->Initialize(); // New connection
        // TODO SetSegSize(segsize);
      }
      flow->SetForwardTarget(MakeCallback(&ADDCNL3_5Protocol::ForwardDown, this));
      //flow->SetRoute(route);

      // set packet size before forward down
      c3Tag.SetPacketSize (GetPacketSize (packet, protocol));
      packet->AddPacketTag (c3Tag);
      flow->Send(packet, route);
      /*
      Ptr<ADDCNTunnel> tunnel = ADDCNDivision::GetDivision (c3Tag.GetTenantId (), c3Tag.GetType ())->GetTunnel (src, dst);
      tunnel->SetForwardTarget (MakeCallback (&ADDCNL3_5Protocol::ForwardDown, this));
      tunnel->SetRoute (route);
      tunnel->Send (packet, protocol);
      */
    }
    // not a data packet: ACK or sth else
    // TODO
    ForwardDown (packet, src, dst, protocol, route);
  }
  // check tag before send
}

void
ADDCNL3_5Protocol::Send6 (Ptr<Packet> packet,
                       Ipv6Address src, Ipv6Address dst,
                       uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << src << dst << (uint32_t)protocol << route);

  Ptr<Packet> copy = packet->Copy ();
  ForwardDown6 (copy, src, dst, protocol, route);
}

enum IpL4Protocol::RxStatus
ADDCNL3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &header,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);

  C3Tag c3Tag;
  if (packet->PeekPacketTag (c3Tag))
  {
  // TODO Extract FiveTuple tuple from ipHeader & tcpHeader
  // Ptr<ADDCNFlow> flow = ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(tuple);
  // if(tcpHeader.GetFlags() & TCPHeader::SYN)
  // {
  //    flow->Initialize();
  //    flow->SetSegSize();
  // }   
  // else
  // {
  //    flow->ecn_recorder->NotifyReceived(header);
        C3EcnRecorder::GetEcnRecorder (c3Tag.GetTenantId (), c3Tag.GetType (),
                                     header.GetSource (), header.GetDestination ())->NotifyReceived (header);
  //    if(tcpHeader.GetFlags() & (TCPHeader::ACK | TCPHeader::SYN) == TCPHeader::ACK)
  //    {
  //      flow->UpdateReceiveWindow();
  //    }
  // }
  // flow->SetReceiveWindow(packet);
  }
  return ForwardUp (packet, header, incomingInterface, header.GetProtocol ());
}

enum IpL4Protocol::RxStatus
ADDCNL3_5Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &header,
                         Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress ());
  return ForwardUp6 (packet, header, incomingInterface, header.GetNextHeader ());
}

void
ADDCNL3_5Protocol::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  IpL3_5Protocol::DoDispose ();
}

uint32_t
ADDCNL3_5Protocol::GetPacketSize (Ptr<Packet> packet, uint8_t protocol)
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
