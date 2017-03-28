#include "c3-flow.h"

#include "ns3/log.h"

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
    m_sentSize (0),
    m_bufferedSize (0),
    m_weight (0),
    m_protocol (0),
    m_tbf (CreateObject<TokenBucketFilter> ())
{
  NS_LOG_FUNCTION (this);
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
  NS_LOG_FUNCTION (this << (uint32_t)protocol);
  m_protocol = protocol;
}

void
C3Flow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_flowSize = c3Tag.GetFlowSize ();
  m_bufferedSize += c3Tag.GetPacketSize ();
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
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_sentSize += c3Tag.GetPacketSize ();
  m_bufferedSize -= c3Tag.GetPacketSize ();
  m_forwardTarget (packet, m_protocol);
}

void
C3Flow::Drop (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this <<packet);
  NS_LOG_INFO ("Packet drop: " << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_bufferedSize -= c3Tag.GetPacketSize ();
}

} //namespace dcn
} //namespace ns3
