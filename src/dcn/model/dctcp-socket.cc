#include "dctcp-socket.h"

#include "ns3/log.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DctcpSocket");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (DctcpSocket);

TypeId
DctcpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::DctcpSocket")
      .SetParent<TcpSocketBase> ()
      .SetGroupName ("DCN")
      .AddConstructor<DctcpSocket> ()
      .AddAttribute ("DCTCPWeight",
                     "Weigt for calculating DCTCP's alpha parameter",
                     DoubleValue (1.0 / 16.0),
                     MakeDoubleAccessor (&DctcpSocket::m_g),
                     MakeDoubleChecker<double> (0.0, 1.0))
      .AddTraceSource ("DCTCPAlpha",
                       "Alpha parameter stands for the congestion status",
                       MakeTraceSourceAccessor (&DctcpSocket::m_alpha),
                       "ns3::TracedValueCallback::Double");
  return tid;
}

TypeId
DctcpSocket::GetInstanceTypeId () const
{
  return DctcpSocket::GetTypeId ();
}

DctcpSocket::DctcpSocket (void)
  : TcpSocketBase (),
    m_g (1.0 / 16.0),
    m_alpha (1.0),
    m_ackedBytesEcn (0),
    m_ackedBytesTotal (0),
    m_alphaUpdateSeq (0),
    m_dctcpMaxSeq (0),
    m_ceTransition (false)
{
    m_ecn = true;
}

DctcpSocket::DctcpSocket (const DctcpSocket &sock)
  : TcpSocketBase (sock),
    m_g (sock.m_g),
    m_alpha (sock.m_alpha),
    m_ackedBytesEcn (sock.m_ackedBytesEcn),
    m_ackedBytesTotal (sock.m_ackedBytesTotal),
    m_alphaUpdateSeq (sock.m_alphaUpdateSeq),
    m_dctcpMaxSeq (sock.m_dctcpMaxSeq),
    m_ceTransition (sock.m_ceTransition)
{
    m_ecn = true;
}

void
DctcpSocket::SendAckPacket (void)
{
  NS_LOG_FUNCTION (this);

  if (m_ecnReceiverState != ECN_DISABLED)
    {
      if (m_ceTransition)
        {
          NS_LOG_DEBUG ("Send ECE packet back");
          SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
          if (m_ecnReceiverState != ECN_CE_RCVD)
            {
              m_ceTransition = false;
            }
          m_ecnReceiverState = ECN_ECE_SENT;
        }
      else
        {
          SendEmptyPacket (TcpHeader::ACK);
          if (m_ecnReceiverState == ECN_CE_RCVD)
            {
              m_ceTransition = true;
            }
          m_ecnReceiverState = ECN_IDLE;
        }
    }
  else
    {
      SendEmptyPacket (TcpHeader::ACK);
    }
}

void
DctcpSocket::EstimateRtt (const TcpHeader &tcpHeader)
{
  UpdateAlpha (tcpHeader);
  TcpSocketBase::EstimateRtt (tcpHeader);
}

void
DctcpSocket::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                               bool isRetransmission)
{
  // set dctcp max seq to highTxMark
  m_dctcpMaxSeq =std::max (m_tcb->m_highTxMark.Get (), std::max (seq + sz, m_dctcpMaxSeq));
  TcpSocketBase::UpdateRttHistory (seq, sz, isRetransmission);
}

void
DctcpSocket::UpdateAlpha (const TcpHeader &tcpHeader)
{
  NS_LOG_FUNCTION (this);
  int32_t ackedBytes = tcpHeader.GetAckNumber () - m_highRxAckMark.Get ();
  if (ackedBytes > 0)
    {
      m_ackedBytesTotal += ackedBytes;
      if (tcpHeader.GetFlags () & TcpHeader::ECE)
        {
          m_ackedBytesEcn += ackedBytes;
        }
    }
  /*
   * check for barrier indicating its time to recalculate alpha.
   * this code basically updated alpha roughly once per RTT.
   */
  if (tcpHeader.GetAckNumber () > m_alphaUpdateSeq)
    {
      m_alphaUpdateSeq = m_dctcpMaxSeq;
      m_alpha = (1 - m_g) * m_alpha + m_g * ((double)m_ackedBytesEcn / (m_ackedBytesTotal ? m_ackedBytesTotal : 1));
      m_ackedBytesEcn = m_ackedBytesTotal = 0;
    }
}

void
DctcpSocket::Retransmit (void)
{
  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    {
      return;
    }
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer->HeadSequence () >= m_tcb->m_highTxMark)
    {
      return;
    }

  /*
   * When a TCP sender detects segment loss using the retransmission timer
   * and the given segment has not yet been resent by way of the
   * retransmission timer, the value of ssthresh MUST be set to no more
   * than the value given in equation (4):
   *
   *   ssthresh = max (FlightSize / 2, 2*SMSS)            (4)
   *
   * where, as discussed above, FlightSize is the amount of outstanding
   * data in the network.
   *
   * On the other hand, when a TCP sender detects segment loss using the
   * retransmission timer and the given segment has already been
   * retransmitted by way of the retransmission timer at least once, the
   * value of ssthresh is held constant.
   *
   * Conditions to decrement slow - start threshold are as follows:
   *
   * *) The TCP state should be less than disorder, which is nothing but open.
   * If we are entering into the loss state from the open state, we have not yet
   * reduced the slow - start threshold for the window of data. (Nat: Recovery?)
   * *) If we have entered the loss state with all the data pointed to by high_seq
   * acknowledged. Once again it means that in whatever state we are (other than
   * open state), all the data from the window that got us into the state, prior to
   * retransmission timer expiry, has been acknowledged. (Nat: How this can happen?)
   * *) If the above two conditions fail, we still have one more condition that can
   * demand reducing the slow - start threshold: If we are already in the loss state
   * and have not yet retransmitted anything. The condition may arise in case we
   * are not able to retransmit anything because of local congestion.
   */

  if (m_tcb->m_congState != TcpSocketState::CA_LOSS)
    {
      m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
      m_tcb->m_congState = TcpSocketState::CA_LOSS;
      m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, BytesInFlight ());
      m_tcb->m_cWnd = m_tcb->m_segmentSize;
    }

  m_tcb->m_nextTxSequence = m_txBuffer->HeadSequence (); // Restart from highest Ack
  m_dupAckCount = 0;

  // reset dctcp seq value if retransmit (why?)
  m_alphaUpdateSeq = m_dctcpMaxSeq = m_tcb->m_nextTxSequence;

  NS_LOG_DEBUG ("RTO. Reset cwnd to " <<  m_tcb->m_cWnd << ", ssthresh to " <<
                m_tcb->m_ssThresh << ", restart from seqnum " << m_tcb->m_nextTxSequence);
  DoRetransmit ();                          // Retransmit the packet
}

Ptr<TcpSocketBase>
DctcpSocket::Fork (void)
{
  return CopyObject<DctcpSocket> (this);
}

void
DctcpSocket::ProcessEcnState (const TcpHeader &tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  if (m_ecnReceiverState == ECN_CE_RCVD)
    {
      // 0->1
      if (!m_ceTransition)
        {
          // send immediate ACK with ECN = 0
          m_delAckCount = m_delAckMaxCount;
        }
    }
  else
    {
      // 1->0
      if (m_ceTransition)
        {
          // send immediate ACK with ECN = 1
          m_delAckCount = m_delAckMaxCount;
        }
    }
}

void
DctcpSocket::HalveCwnd (void)
{
  NS_LOG_FUNCTION (this);
  m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, BytesInFlight ());
  // cut down cwnd according to DCTCP algo
  NS_LOG_DEBUG ("Before Cutdown cwnd: " << m_tcb->m_cWnd);
  m_tcb->m_cWnd = std::max ((uint32_t)(m_tcb->m_cWnd * (1 - m_alpha / 2.0)), m_tcb->m_segmentSize);
  NS_LOG_DEBUG ("After Cutdown cwnd: " << m_tcb->m_cWnd);
}

} // namespace dcn
} // namespace ns3

