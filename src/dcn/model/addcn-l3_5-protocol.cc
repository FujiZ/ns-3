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

  bool send = true;
  if(protocol == 6) // TCP
  {
    TcpHeader tcpHeader;
    packet->PeekHeader (tcpHeader);

    NS_LOG_FUNCTION (this << tcpHeader);

    ADDCNFlow::FiveTuple tuple;
    tuple.sourceAddress = src;
    tuple.destinationAddress = dst;
    tuple.protocol = protocol;
    tuple.sourcePort = tcpHeader.GetSourcePort ();
    tuple.destinationPort = tcpHeader.GetDestinationPort ();

    /*
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
    */

    C3Tag c3Tag;
    if(packet->RemovePacketTag (c3Tag))
    { // From Sender side, even EMPTY packets like ACK, SYN. Done through socket level send trace
      Ptr<ADDCNFlow> flow = ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(tuple, &c3Tag);
      //double scale = ((double)(c3Tag.GetTenantId() + 1)) / 4; // three flows start concurrently
      //flow->UpdateScale(scale);
      //double scale = ((double)(c3Tag.GetTenantId() + 1)) / 6.5; // Four flows start concurrently
      //flow->UpdateScale(scale);
      // ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->SetWeight(c3Tag.GetTenantId() + 1);
      /*
      double size = pow(2, ADDCNSlice::GetSliceNumber() - 1);
      double size;
      if(ADDCNSlice::GetSliceNumber() <= 2)
        size = 2;
      else
        size = 4;
      double scale = ((double)(c3Tag.GetTenantId() + 1)) / size; 
      flow->UpdateScale(scale);
      */
      /*
      double scaledWeight = flow->GetWeight() * ((double)(c3Tag.GetTenantId())) / 4.0; 
      double fs_weight = flow->GetScaledWeight();
      if(fabs(scaledWeight - fs_weight) > 0.01)
        {
          printf("Updating scale for tenant %d to %.2f", c3Tag.GetTenantId(), scale);
        }
      */
      //if((tcpHeader.GetFlags() & (TcpHeader::SYN | TcpHeader::ACK)) == TcpHeader::SYN)
      //{
      //  flow->Initialize(); // New connection
      //  flow->SetSegmentSize(c3Tag.GetSegmentSize());
      //}
      //flow->SetForwardTarget(MakeCallback(&ADDCNL3_5Protocol::ForwardDown, this));
      //flow->SetRoute(route);

      // set packet size before forward down
      c3Tag.SetPacketSize (GetPacketSize (packet, protocol));
      packet->AddPacketTag (c3Tag);
      send = flow->NotifySend(packet);
      //return;
      /*
      Ptr<ADDCNTunnel> tunnel = ADDCNDivision::GetDivision (c3Tag.GetTenantId (), c3Tag.GetType ())->GetTunnel (src, dst);
      tunnel->SetForwardTarget (MakeCallback (&ADDCNL3_5Protocol::ForwardDown, this));
      tunnel->SetRoute (route);
      tunnel->Send (packet, protocol);
      */
    }
    else
    {
      NS_LOG_DEBUG("NO C3TAG");
      ADDCNFlow::FiveTuple rtuple;
      rtuple.sourceAddress = dst;
      rtuple.destinationAddress = src;
      rtuple.protocol = protocol;
      rtuple.sourcePort = tcpHeader.GetDestinationPort ();
      rtuple.destinationPort = tcpHeader.GetSourcePort ();
      Ptr<ADDCNFlow> flow = ADDCNSlice::GetSliceFromTuple(rtuple)->GetFlow(tuple);
      send = flow->NotifySend(packet);
    }
  }
  // check tag before send
  if (send)
    ForwardDown (packet, src, dst, protocol, route);
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
                         Ipv4Header const &ipHeader,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << ipHeader);
  uint8_t protocol = ipHeader.GetProtocol ();

  bool receive = true;
  if(protocol == 6) // TCP
  {
    C3Tag c3Tag;
    TcpHeader tcpHeader;
    packet->PeekHeader (tcpHeader);

    ADDCNFlow::FiveTuple tuple, rtuple;
    tuple.sourceAddress = ipHeader.GetSource ();
    tuple.destinationAddress = ipHeader.GetDestination ();
    tuple.protocol = ipHeader.GetProtocol ();
    tuple.sourcePort = tcpHeader.GetSourcePort ();
    tuple.destinationPort = tcpHeader.GetDestinationPort ();

    rtuple.sourceAddress = ipHeader.GetDestination ();
    rtuple.destinationAddress = ipHeader.GetSource ();
    rtuple.protocol = ipHeader.GetProtocol ();
    rtuple.sourcePort = tcpHeader.GetDestinationPort ();
    rtuple.destinationPort = tcpHeader.GetSourcePort ();

    if (packet->PeekPacketTag (c3Tag))
    { // At the receiver side
#ifdef DCTCPACK
      Ptr<ADDCNFlow> rflow = ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(rtuple);
      receive = rflow->NotifyReceive (packet, ipHeader);
#else
      Ptr<ADDCNFlow> flow = ADDCNSlice::GetSlice(c3Tag.GetTenantId(), c3Tag.GetType())->GetFlow(tuple);
      //if((tcpHeader.GetFlags() & TcpHeader::SYN) == TcpHeader::SYN)
      //  flow->UpdateEcnStatistics(tcpHeader); // TO closely track dctcp
      //else
        flow->UpdateEcnStatistics(ipHeader);
#endif
    }
    else // TODO: What if both side supports C3Tag?
    { // At the sender side
      Ptr<ADDCNFlow> rflow = ADDCNSlice::GetSliceFromTuple(rtuple)->GetFlow(rtuple);
      receive = rflow->NotifyReceive (packet, ipHeader);

    /*
    //if((tcpHeader.GetFlags() & TcpHeader::SYN) == TcpHeader::SYN)
    if((tcpHeader.GetFlags() & (TcpHeader::ACK | TcpHeader::SYN)) == TcpHeader::SYN)
    {
      NS_LOG_DEBUG("Receive side, SYN");
      rflow->Initialize(); // New connection
      rflow->SetSegmentSize(c3Tag.GetSegmentSize());
      if(tcpHeader.HasOption (TcpOption::WINSCALE))
      {
        NS_LOG_DEBUG("Receive side, SYN, WScale");
        rflow->ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
      }
    }
    */
  /*
    if((tcpHeader.GetFlags() & TcpHeader::SYN) == TcpHeader::SYN)
    {
      NS_LOG_DEBUG("Receive side, SYN");
      if(tcpHeader.HasOption (TcpOption::WINSCALE))
      {
        NS_LOG_DEBUG("Receive side, SYN & ACK, WScale");
        rflow->ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
      }

      rflow->NotifyReceived(tcpHeader);
      rflow->UpdateReceiveWindow(tcpHeader);
    }
    else
    {
      rflow->NotifyReceived(tcpHeader);
      rflow->UpdateReceiveWindow(tcpHeader);
      rflow->SetReceiveWindow(packet);
    }
*/
      //packet->PeekHeader (tcpHeader);
      //NS_LOG_FUNCTION(this << "Window Set Header " << tcpHeader);
    }
  }
  if (receive)
    return ForwardUp (packet, ipHeader, incomingInterface, ipHeader.GetProtocol ());
  else
    return IpL4Protocol::RX_OK;
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
