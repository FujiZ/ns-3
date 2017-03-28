#include "addcn-flow.h"

#include "ns3/log.h"
#include "ns3/tcp-header.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADDCNFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADDCNFlow);

bool operator < (const ADDCNFlow::FiveTuple &t1,
                 const ADDCNFlow::FiveTuple &t2)
{
  if (t1.sourceAddress < t2.sourceAddress)
    {
      return true;
    }
  if (t1.sourceAddress != t2.sourceAddress)
    {
      return false;
    }

  if (t1.destinationAddress < t2.destinationAddress)
    {
      return true;
    }
  if (t1.destinationAddress != t2.destinationAddress)
    {
      return false;
    }

  if (t1.protocol < t2.protocol)
    {
      return true;
    }
  if (t1.protocol != t2.protocol)
    {
      return false;
    }

  if (t1.sourcePort < t2.sourcePort)
    {
     return true;
    }
  if (t1.sourcePort != t2.sourcePort)
    {
      return false;
    }

  if (t1.destinationPort < t2.destinationPort)
    {
      return true;
    }
  if (t1.destinationPort != t2.destinationPort)
    {
      return false;
    }

  return false;
}

bool operator == (const ADDCNFlow::FiveTuple &t1,
                  const ADDCNFlow::FiveTuple &t2)
{
  return (t1.sourceAddress      == t2.sourceAddress &&
          t1.destinationAddress == t2.destinationAddress &&
          t1.protocol           == t2.protocol &&
          t1.sourcePort         == t2.sourcePort &&
          t1.destinationPort    == t2.destinationPort);
}

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
    m_sentSize (0),
    m_alpha (0.0),
    m_scale (1.0),
    m_weight (0.0),
    m_weightScaled(0.0)
{
  NS_LOG_FUNCTION (this);
  //m_tbf->SetSendTarget (MakeCallback (&ADDCNFlow::Forward, this));
  //m_tbf->SetDropTarget (MakeCallback (&ADDCNFlow::Drop, this));
}

ADDCNFlow::~ADDCNFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
ADDCNFlow::Initialize ()
{
  m_flowSize = 0;
  m_sentSize = 0;
  m_alpha = 0;
  //m_scale = 1.0;
  m_weight = 0.0;
  m_segSize = 0;
  m_weightScaled = 0.0;
}

void
ADDCNFlow::SetSegmentSize(int32_t size)
{
  m_segSize = size;
}

void
ADDCNFlow::NotifyReceived(const Ipv4Header &header)
{
  m_ecnRecorder->NotifyReceived(header);
}

void
ADDCNFlow::UpdateReceiveWindow()
{
  if(m_alpha > 10e-7)
    m_rwnd = m_rwnd * (1 - m_alpha/2);
  else
    m_rwnd += m_weightScaled * m_segSize;
}

void
ADDCNFlow::SetReceiveWindow(Ptr<Packet> &packet)
{
  // TODO
  TcpHeader tcpHeader;
  uint32_t bytesRemoved = packet->RemoveHeader(tcpHeader);
  if(bytesRemoved == 0)
  {
    NS_LOG_ERROR("SetReceiveWindow bytes remoed invalid");
    return;
  }
  // TODO check whether valid
  tcpHeader.SetWindowSize(m_rwnd);
  packet->AddHeader(tcpHeader);
  return;
}

void
ADDCNFlow::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

void 
ADDCNFlow::SetFiveTuple (ADDCNFlow::FiveTuple tuple)
{
  NS_LOG_FUNCTION (this);
  m_tuple = tuple;
}

void
ADDCNFlow::Send (Ptr<Packet> packet, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << packet);
  NS_LOG_INFO ("Packet sent: " << packet);

  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_flowSize = c3Tag.GetFlowSize ();
  m_sentSize += c3Tag.GetPacketSize ();
  m_forwardTarget (packet, m_tuple.sourceAddress, m_tuple.destinationAddress, m_tuple.protocol, route);
}

double
ADDCNFlow::GetWeight (void) const
{
  return m_weight;
}

void
ADDCNFlow::UpdateScale(const double s)
{
  NS_LOG_FUNCTION (this << s);
  if(s > 10e-7)
  {
    m_scale = s;
    m_weightScaled = m_weight / m_scale;
  }
}

void
ADDCNFlow::UpdateAlpha()
{
  NS_LOG_FUNCTION (this);
  m_alpha = (1 - m_g) * m_alpha + m_g * m_ecnRecorder->GetRatio ();
}

void
ADDCNFlow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget.Nullify ();
  Object::DoDispose ();
}

} //namespace dcn
} //namespace ns3
