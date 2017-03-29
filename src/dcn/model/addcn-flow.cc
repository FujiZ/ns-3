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
    m_g (1.0/16.0),
    m_alpha (0.0),
    m_scale (1.0),
    m_weight (0.0),
    m_weightScaled(0.0),
    m_seqNumber(0),
    m_updateRwndSeq(0),
    m_updateAlphaSeq(0),
    m_sndWindShift(0)
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
  m_g = 1.0 / 16.0;
  m_alpha = 0;
  //m_scale = 1.0;
  m_weight = 0.0;
  m_segSize = 0;
  m_weightScaled = 0.0;
  m_seqNumber = 0;
  m_updateRwndSeq = 0;
  m_updateAlphaSeq = 0;
  m_sndWindShift = 0;
}

void
ADDCNFlow::SetSegmentSize(uint32_t size)
{
  m_segSize = size;
}


void
ADDCNFlow::UpdateEcnStatistics(const Ipv4Header &header)
{
  m_ecnRecorder->NotifyReceived(header);
}

void
ADDCNFlow::NotifyReceived(const TcpHeader &tcpHeader)
{
  SequenceNumber32 curSeq = tcpHeader.GetAckNumber ();
  // curSeq > m_updateAlphaSeq ensures updating alpha only one time every RTT
  if(curSeq > m_updateAlphaSeq)
  {
    m_updateAlphaSeq = m_seqNumber;
    m_alpha = (1 - m_g) * m_alpha + m_g * m_ecnRecorder->GetRatio ();
    m_ecnRecorder->Reset();
  }
}

void
ADDCNFlow::UpdateReceiveWindow(const TcpHeader &tcpHeader)
{
  SequenceNumber32 curSeq = tcpHeader.GetAckNumber ();
  // curSeq > m_updateRwndSeq ensures that this action is operated only one time every RTT
  if(m_alpha > 10e-7 && curSeq > m_updateRwndSeq)
  {
    m_updateRwndSeq = m_seqNumber;
    m_rwnd = m_rwnd * (1 - m_alpha/2);
  }
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
  // TODO check WScale option
  uint32_t w = m_rwnd >> m_sndWindShift;
  
  tcpHeader.SetWindowSize(w);
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
  TcpHeader tcpHeader;
  packet->PeekHeader (tcpHeader);
  this->m_seqNumber = tcpHeader.GetSequenceNumber ();
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

SequenceNumber32 
ADDCNFlow::GetHighSequenceNumber()
{
  return m_seqNumber;
}

void
ADDCNFlow::ProcessOptionWScale (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);
 
  Ptr<const TcpOptionWinScale> ws = DynamicCast<const TcpOptionWinScale> (option);

  // In naming, we do the contrary of RFC 1323. The received scaling factor
  // is Rcv.Wind.Scale (and not Snd.Wind.Scale)
  m_sndWindShift = ws->GetScale ();

  if (m_sndWindShift > 14)
  {
    NS_LOG_WARN ("Possible error; m_sndWindShift exceeds 14: " << m_sndWindShift);
    m_sndWindShift = 14;
  }

  NS_LOG_INFO (" Received a scale factor of " <<
               static_cast<int> (m_sndWindShift));
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
