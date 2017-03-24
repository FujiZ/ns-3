#include "addcn-flow.h"

#include "ns3/log.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADDCNFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADDCNFlow);

TypeId
ADDCNFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADDCNFlow")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

ADDCNFlow::ADDCNFlow ()
  : m_flowSize (0),
    m_sendedSize (0),
    m_bufferedSize (0),
    m_weight (0),
    m_protocol (0),
    m_tbf (CreateObject<TokenBucketFilter> ())
{
  NS_LOG_FUNCTION (this);
  m_tbf->SetSendTarget (MakeCallback (&ADDCNFlow::Forward, this));
  m_tbf->SetDropTarget (MakeCallback (&ADDCNFlow::Drop, this));
}

ADDCNFlow::~ADDCNFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
ADDCNFlow::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

void
ADDCNFlow::SetProtocol (uint8_t protocol)
{
  NS_LOG_FUNCTION (this << (uint32_t)protocol);
  m_protocol = protocol;
}

void
ADDCNFlow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_flowSize = c3Tag.GetFlowSize ();
  m_bufferedSize += c3Tag.GetPacketSize ();
  m_tbf->Send (packet);
}

double
ADDCNFlow::GetWeight (void) const
{
  return m_weight;
}

void
ADDCNFlow::SetRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_tbf->SetRate (rate);
}

void
ADDCNFlow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget.Nullify ();
  m_tbf = 0;
  Object::DoDispose ();
}

void
ADDCNFlow::Forward (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  NS_LOG_INFO ("Packet sent: " << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_sendedSize += c3Tag.GetPacketSize ();
  m_bufferedSize -= c3Tag.GetPacketSize ();
  m_forwardTarget (packet, m_protocol);
}

void
ADDCNFlow::Drop (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this <<packet);
  NS_LOG_INFO ("Packet drop: " << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_bufferedSize -= c3Tag.GetPacketSize ();
}

} //namespace dcn
} //namespace ns3
