#include "l2dct-socket.h"

#include "ns3/log.h"

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("L2dctSocket");

NS_OBJECT_ENSURE_REGISTERED (L2dctSocket);

TypeId
L2dctSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::L2dctSocket")
      .SetParent<DctcpSocket> ()
      .SetGroupName ("Internet")
      .AddConstructor<L2dctSocket> ()
      .AddAttribute ("WeightMax",
                     "Max weight a flow can get.",
                     DoubleValue (2.5),
                     MakeDoubleAccessor (&L2dctSocket::m_weightMax),
                     MakeDoubleChecker<double> (0.0))
      .AddAttribute ("WeightMin",
                     "Min weight a flow can get.",
                     DoubleValue (0.125),
                     MakeDoubleAccessor (&L2dctSocket::m_weightMin),
                     MakeDoubleChecker<double> (0.0))
      .AddTraceSource ("SentBytes",
                       "Bytes already sent.",
                       MakeTraceSourceAccessor (&L2dctSocket::m_sentBytes),
                       "ns3::TracedValueCallback::Uint64");
  return tid;
}

TypeId
L2dctSocket::GetInstanceTypeId () const
{
  return L2dctSocket::GetTypeId ();
}

L2dctSocket::L2dctSocket (void)
  : DctcpSocket (),
    m_weightMax (2.5),
    m_weightMin (0.125),
    m_sentBytes (0)
{
}

L2dctSocket::L2dctSocket (const L2dctSocket &sock)
  : DctcpSocket (sock),
    m_weightMax (sock.m_weightMax),
    m_weightMin (sock.m_weightMin),
    m_sentBytes (sock.m_sentBytes)
{
}

void
L2dctSocket::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz, bool isRetransmission)
{
  NS_LOG_FUNCTION (this);

  // set dctcp max seq to highTxMark
  m_dctcpMaxSeq =std::max (std::max (seq + sz, m_tcb->m_highTxMark.Get ()), m_dctcpMaxSeq);

  // update the history of sequence numbers used to calculate the RTT
  if (isRetransmission == false)
    { // This is the next expected one, just log at end
      m_history.push_back (RttHistory (seq, sz, Simulator::Now ()));
      m_sentBytes += sz;
    }
  else
    { // This is a retransmit, find in list and mark as re-tx
      for (RttHistory_t::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
          if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
              i->retx = true;

              m_sentBytes -= i->count;
              i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
              m_sentBytes += i->count;
              break;
            }
        }
    }
}

Ptr<TcpSocketBase>
L2dctSocket::Fork (void)
{
  return CopyObject<L2dctSocket> (this);
}

uint32_t
L2dctSocket::GetSsThresh (void)
{
  NS_LOG_FUNCTION (this);

  double weightC = GetWeightC ();
  double b = std::pow (m_alpha, weightC);
  uint32_t newWnd = (1 - b / 2) * m_tcb->m_cWnd;
  return std::max (newWnd, 2 * m_tcb->m_segmentSize);
}

void
L2dctSocket::IncreaseWindow (uint32_t segmentAcked)
{
  NS_LOG_FUNCTION (this << segmentAcked);

  if (m_tcb->m_cWnd < m_tcb->m_ssThresh)
    {
      segmentAcked = SlowStart (segmentAcked);
    }

  if (m_tcb->m_cWnd >= m_tcb->m_ssThresh)
    {
      CongestionAvoidance (segmentAcked);
    }
}

uint32_t
L2dctSocket::SlowStart (uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << segmentsAcked);
  if (segmentsAcked >= 1)
    {
      m_tcb->m_cWnd += m_tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_tcb->m_cWnd << " ssthresh " << m_tcb->m_ssThresh);
      return segmentsAcked - 1;
    }
  return 0;
}

void
L2dctSocket::CongestionAvoidance (uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double weightC = GetWeightC ();

      double k = weightC / m_weightMax;
      double adder = k * m_tcb->m_segmentSize * m_tcb->m_segmentSize / m_tcb->m_cWnd.Get ();
      m_tcb->m_cWnd += static_cast<uint32_t> (std::round (std::max (1.0, adder)));
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_tcb->m_cWnd <<
                   " ssthresh " << m_tcb->m_ssThresh);
    }
}

double
L2dctSocket::GetWeightC (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t segCount = m_sentBytes / m_tcb->m_segmentSize;

  double weightC = segCount <= 200 ? m_weightMax : (m_weightMax - (m_weightMax - m_weightMin) * (segCount - 200) / 800);
  return std::max (std::min (weightC, m_weightMax), m_weightMin);
}

} // namespace ns3
