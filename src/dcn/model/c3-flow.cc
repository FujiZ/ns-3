#include "c3-flow.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Flow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Flow);

TypeId
C3Flow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Flow")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3Flow::C3Flow ()
  : m_flowSize (0),
    m_sentBytes (0),
    m_bufferedBytes (0),
    m_weight (0.0),
    m_protocol (0)
{
  NS_LOG_FUNCTION (this);
  m_tbf = CreateObject<TokenBucketFilter> ();
  m_tbf->SetSendTarget (MakeCallback (&C3Flow::Forward, this));
  m_tbf->SetDropTarget (MakeCallback (&C3Flow::Drop, this));
}

C3Flow::~C3Flow ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Flow::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

void
C3Flow::SetProtocol (uint8_t protocol)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (protocol));
  m_protocol = protocol;
}

void
C3Flow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  bool retval = packet->RemovePacketTag (c3Tag);
  NS_ASSERT (retval);
  c3Tag.SetPacketSize (GetPacketSize (packet));
  m_flowSize = c3Tag.GetFlowSize ();
  m_bufferedBytes += c3Tag.GetPacketSize ();
  packet->AddPacketTag (c3Tag);
  m_tbf->Send (packet);
}

double
C3Flow::GetWeight (void) const
{
  return m_weight;
}

void
C3Flow::SetRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_tbf->SetRate (rate);
}

void
C3Flow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget.Nullify ();
  m_tbf = 0;
  Object::DoDispose ();
}

void
C3Flow::Forward (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  NS_LOG_INFO ("Packet sent: " << packet);
  C3Tag c3Tag;
  bool retval = packet->PeekPacketTag (c3Tag);
  NS_ASSERT (retval);
  m_sentBytes += c3Tag.GetPacketSize ();
  m_bufferedBytes -= c3Tag.GetPacketSize ();
  m_forwardTarget (packet, m_protocol);
}

void
C3Flow::Drop (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this <<packet);
  NS_LOG_INFO ("Packet drop: " << packet);
  C3Tag c3Tag;
  bool retval = packet->PeekPacketTag (c3Tag);
  NS_ASSERT (retval);
  m_bufferedBytes -= c3Tag.GetPacketSize ();
}

uint32_t
C3Flow::GetPacketSize (Ptr<const Packet> packet) const
{
  // the calculation of packet size can be placed here
  uint32_t size;
  switch (m_protocol) {
    case 6: // TCP
      {
        TcpHeader tcpHeader;
        packet->PeekHeader (tcpHeader);
        size = packet->GetSize () - tcpHeader.GetSerializedSize ();
        break;
      }
    case 17: // UDP
      {

        UdpHeader udpHeader;
        packet->PeekHeader (udpHeader);
        size = packet->GetSize () - udpHeader.GetSerializedSize ();
        break;
      }
    default:
      {
        NS_ABORT_MSG ("Protocol " << static_cast<uint32_t> (m_protocol) << "not implemented");
        break;
      }
    }
  return size;

}

} //namespace dcn
} //namespace ns3
