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
  : m_rwnd (0),
    m_flowSize (0),
    m_sentSize (0),
    m_g (1.0/16.0),
    m_alpha (0.0),
    m_scale (1.0),
    m_weight (1.0),
    m_winScalingEnabled (true),
    m_timestampEnabled(true),
    m_weightScaled(1.0),
    m_seqNumber(0),
    m_updateRwndSeq(0),
    m_updateAlphaSeq(0),
    m_sndWindShift(0),
    m_recover(0)
{
  NS_LOG_FUNCTION (this);
  m_ecnRecorder = CreateObject<C3EcnRecorder> ();
  m_tcb      = CreateObject<TcpSocketState> ();
  m_rtt      = CreateObject<RttMeanDeviation> ();

  Initialize();
  //m_tbf->SetSendTarget (MakeCallback (&ADDCNFlow::Forward, this));
  //m_tbf->SetDropTarget (MakeCallback (&ADDCNFlow::Drop, this));
}

ADDCNFlow::~ADDCNFlow ()
{
  NS_LOG_FUNCTION (this);
  m_retxEvent.Cancel ();
}

void
ADDCNFlow::Initialize ()
{
  m_rwnd = 0;
  m_flowSize = 0;
  m_sentSize = 0;
  m_g = 1.0 / 16.0;
  m_alpha = 0;
  m_scale = 1.0;
  m_weight = 1.0;
  m_winScalingEnabled = true;
  m_timestampEnabled = true;
  m_segSize = 0;
  m_weightScaled = 1.0;
  m_seqNumber = 0;
  m_updateRwndSeq = 0;
  m_updateAlphaSeq = 0;
  m_recover = 0;
  m_sndWindShift = 0;

  m_rto = Seconds(0.0);
  m_minRto = Seconds(1.0); // RFC 6298 says min RTO=1 sec, but Linux uses 200ms
  m_clockGranularity = Seconds (0.001);
  m_lastRtt = Seconds(0.0);

  m_dupAckCount = 0;
  m_retransOut = 0;
  m_bytesAckedNotProcessed = 0;
  m_isFirstPartialAck = true;

  m_ecnState = TcpSocket::ECN_CONN;
  m_ecnRecorder->Reset ();
  m_ecnEchoSeq = 0;

  m_rtt->Reset ();
  m_retxEvent.Cancel ();
  m_tcb->m_initialCWnd = 1;
  m_tcb->m_initialSsThresh = UINT32_MAX;
  m_tcb->m_segmentSize = 536; 
  m_tcb->m_lastAckedSeq = 0;
  m_tcb->m_congState = TcpSocketState::CA_OPEN;
  m_tcb->m_highTxMark = 0;
  // Change m_nextTxSequence for non-zero initial sequence number
  m_tcb->m_nextTxSequence = 0;

  m_tcb->m_cWnd = m_tcb->m_initialCWnd * m_tcb->m_segmentSize;
  m_tcb->m_ssThresh = m_tcb->m_initialSsThresh;
}

void
ADDCNFlow::SetSegmentSize(uint32_t size)
{
  NS_LOG_FUNCTION(this << size);
  m_segSize = size;
  m_tcb->m_segmentSize = size;
  m_tcb->m_cWnd = m_tcb->m_initialCWnd * m_tcb->m_segmentSize;
  m_tcb->m_ssThresh = m_tcb->m_initialSsThresh;
}


void
ADDCNFlow::UpdateEcnStatistics(const Ipv4Header &header)
{
  NS_LOG_FUNCTION(this << header);
  m_ecnRecorder->NotifyReceived(header);
}

void
ADDCNFlow::UpdateAlpha(const TcpHeader &tcpHeader)
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
  NS_LOG_FUNCTION(this << tcpHeader << "SEG_SIZE" << m_segSize << "M_RWND" << m_rwnd << "ALPHA" << m_alpha << "m_weightScaled" << m_weightScaled);
  SequenceNumber32 curSeq = tcpHeader.GetAckNumber ();
  // curSeq > m_updateRwndSeq ensures that this action is operated only one time every RTT
  if(m_alpha > 10e-7 && curSeq > m_updateRwndSeq)
  {
    m_updateRwndSeq = m_seqNumber;
    m_rwnd = m_rwnd * (1 - m_alpha/2);
  }
  else
    m_rwnd += m_weightScaled * m_segSize;
  m_rwnd = m_rwnd > m_segSize ? m_rwnd : m_segSize;
  NS_LOG_FUNCTION(this << tcpHeader << "SEG_SIZE" << m_segSize << "M_RWND" << m_rwnd << "ALPHA" << m_alpha << "m_weightScaled" << m_weightScaled);
}

void
ADDCNFlow::SetReceiveWindow(Ptr<Packet> &packet)
{
  NS_LOG_FUNCTION(this << packet );
  // TODO
  TcpHeader tcpHeader;
  uint32_t bytesRemoved = packet->RemoveHeader(tcpHeader);
  NS_LOG_FUNCTION(this << "[RWND] Initial Header " << tcpHeader);
  if(bytesRemoved == 0)
  {
    NS_LOG_ERROR("SetReceiveWindow bytes remoed invalid");
    return;
  }
  // TODO check whether valid
  // TODO check WScale option
  m_rwnd = m_tcb->m_cWnd;
  uint32_t w = m_rwnd >> m_sndWindShift;
  
  if(w < tcpHeader.GetWindowSize())
    tcpHeader.SetWindowSize(w);
  packet->AddHeader(tcpHeader);
  NS_LOG_FUNCTION(this << "[RWND] Altered Header " << tcpHeader);
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
ADDCNFlow::NotifySend (Ptr<Packet>& packet, TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  uint32_t bytesRemoved = packet->RemoveHeader(tcpHeader);
  if(bytesRemoved == 0)
  {
    NS_LOG_ERROR("SetReceiveWindow bytes remoed invalid");
    return;
  }
  uint8_t flags = tcpHeader.GetFlags();
  m_seqNumber = tcpHeader.GetSequenceNumber ();

  uint32_t sz = c3Tag.GetPacketSize ();
  m_flowSize = c3Tag.GetFlowSize ();

  m_sentSize += sz;
  
  NS_LOG_FUNCTION (this << "PacketSize" << sz);
  if((flags & TcpHeader::SYN) == TcpHeader::SYN)
  {
    Initialize(); // New connection
    SetSegmentSize(c3Tag.GetSegmentSize());
  }

  if ((m_ecnState & (TcpSocket::ECN_RX_ECHO | TcpSocket::ECN_SEND_CWR)) == TcpSocket::ECN_RX_ECHO)
  {
    NS_LOG_INFO ("Halving CWND duo to receiving ECN Echo.");
    m_ecnState |= TcpSocket::ECN_SEND_CWR;
    HalveCwnd ();
    flags |= TcpHeader::CWR;
    m_ecnState &= ~(TcpSocket::ECN_SEND_CWR | TcpSocket::ECN_RX_ECHO);
    m_ecnEchoSeq = m_seqNumber;
  }

  m_tcb->m_nextTxSequence = std::max (m_tcb->m_nextTxSequence.Get (), m_seqNumber + sz);
  m_tcb->m_highTxMark = std::max (m_seqNumber + sz, m_tcb->m_highTxMark.Get ());
  if (m_seqNumber == m_tcb->m_lastAckedSeq && sz > 0) // Retransmit data packet
  {
    NS_LOG_FUNCTION (this << "Retransmission of data packet detected");
    m_retransOut ++;
  }

  bool isRetransmission = m_seqNumber == m_tcb->m_lastAckedSeq;
  UpdateRttHistory (m_seqNumber, sz, isRetransmission);

  NS_LOG_FUNCTION (this << "m_nextTxSequence = " << m_tcb->m_nextTxSequence
                        << "m_highTxMark = " << m_tcb->m_highTxMark
                        << "m_lastAckedSeq = " << m_tcb->m_lastAckedSeq
                        << "m_retransOut = " << m_retransOut);

  tcpHeader.SetFlags(flags);
  packet->AddHeader(tcpHeader);
  //m_forwardTarget (packet, m_tuple.sourceAddress, m_tuple.destinationAddress, m_tuple.protocol, route);
}

void
ADDCNFlow::NotifyReceive (Ptr<Packet>& packet, TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

    if((tcpHeader.GetFlags() & TcpHeader::SYN) == TcpHeader::SYN)
    {
      NS_LOG_DEBUG("From Receive side, SYN");
      if(tcpHeader.HasOption (TcpOption::WINSCALE))
      {
        NS_LOG_DEBUG("Receive side, SYN, WScale");
        m_winScalingEnabled = true;
        ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
      }
      else
      {
        m_winScalingEnabled = false;
      }

      if(tcpHeader.HasOption (TcpOption::TS))
      {
        NS_LOG_DEBUG("Receive side, SYN, TS");
        m_timestampEnabled = true;
        ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
                                       tcpHeader.GetSequenceNumber ());
      }
      else
      {
        m_timestampEnabled = false;
      }
      
      //UpdateAlpha(tcpHeader);
      //UpdateReceiveWindow(tcpHeader);
    }

    
    
    if((tcpHeader.GetFlags() & TcpHeader::ACK) == TcpHeader::ACK)
    {
      EstimateRtt(tcpHeader);
      UpdateAlpha(tcpHeader);
      ReceivedAck(packet, tcpHeader);
    }
    /*else
    {
      UpdateAlpha(tcpHeader);
      UpdateReceiveWindow(tcpHeader);
      SetReceiveWindow(packet);
    }*/
    if((tcpHeader.GetFlags() & TcpHeader::SYN) == 0)
      SetReceiveWindow(packet);
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
    m_weightScaled = m_weight * m_scale;
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
ADDCNFlow::ProcessOptionTimestamp (const Ptr<const TcpOption> option,
                                       const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this << option);

  /*
  Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);

  if (seq == m_rxBuffer->NextRxSequence () && seq <= m_highTxAck)
    {
      m_timestampToEcho = ts->GetTimestamp ();
    }

  NS_LOG_INFO (m_node->GetId () << " Got timestamp=" <<
               m_timestampToEcho << " and Echo="     << ts->GetEcho ());
  */
}

uint32_t
ADDCNFlow::BytesInFlight ()
{
  // TODO
  NS_LOG_FUNCTION (this);
  // Previous (see bug 1783):
  // uint32_t bytesInFlight = m_highTxMark.Get () - m_txBuffer->HeadSequence ();
  // RFC 4898 page 23
  // PipeSize=SND.NXT-SND.UNA+(retransmits-dupacks)*CurMSS

  // flightSize == UnAckDataCount (), but we avoid the call to save log lines
  uint32_t flightSize = m_tcb->m_nextTxSequence.Get () - m_tcb->m_lastAckedSeq;
  uint32_t duplicatedSize;
  uint32_t bytesInFlight;

  if (m_retransOut > m_dupAckCount)
    {
      duplicatedSize = (m_retransOut - m_dupAckCount)*m_tcb->m_segmentSize;
      bytesInFlight = flightSize + duplicatedSize;
    }
  else
    {
      duplicatedSize = (m_dupAckCount - m_retransOut)*m_tcb->m_segmentSize;
      bytesInFlight = duplicatedSize > flightSize ? 0 : flightSize - duplicatedSize;
    }

  // m_bytesInFlight is traced; avoid useless assignments which would fire
  // fruitlessly the callback
  //if (m_bytesInFlight != bytesInFlight)
  //  {
  //    m_bytesInFlight = bytesInFlight;
  //  }

  return bytesInFlight;
}

uint32_t
ADDCNFlow::GetSsThresh (Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);
  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

void
ADDCNFlow::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz, bool isRetransmission)
{
  NS_LOG_FUNCTION (this);

  // update the history of sequence numbers used to calculate the RTT
  if (isRetransmission == false)
    { // This is the next expected one, just log at end
      m_history.push_back (RttHistory (seq, sz, Simulator::Now ()));
    }
  else
    { // This is a retransmit, find in list and mark as re-tx
      for (RttHistory_t::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
         if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
              i->retx = true;
              i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
              break;
            }
        }
    }
}

void
ADDCNFlow::EstimateRtt (const TcpHeader& tcpHeader)
{
  SequenceNumber32 ackSeq = tcpHeader.GetAckNumber ();
  Time m = Time (0.0);

  // An ack has been received, calculate rtt and log this measurement
  // Note we use a linear search (O(n)) for this since for the common
  // case the ack'ed packet will be at the head of the list
  if (!m_history.empty ())
    {
      RttHistory& h = m_history.front ();
      if (!h.retx && ackSeq >= (h.seq + SequenceNumber32 (h.count)))
        { // Ok to use this sample
          if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
            {
              Ptr<TcpOptionTS> ts;
              ts = DynamicCast<TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));
              m = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());
            }
          else
            {
              m = Simulator::Now () - h.time; // Elapsed time
            }
        }
    }

  // Now delete all ack history with seq <= ack
  while (!m_history.empty ())
    {
      RttHistory& h = m_history.front ();
      if ((h.seq + SequenceNumber32 (h.count)) > ackSeq)
        {
          break;                                                              // Done removing
        }
      m_history.pop_front (); // Remove
    }

  if (!m.IsZero ())
    {
      m_rtt->Measurement (m);                // Log the measurement
      // RFC 6298, clause 2.4
      m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
      m_lastRtt = m_rtt->GetEstimate ();
      NS_LOG_FUNCTION (this << m_lastRtt);
    }
}

void
ADDCNFlow::ReTxTimeout ()
{
  NS_LOG_FUNCTION (this);
  m_recover = m_tcb->m_highTxMark;
  
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
      //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
      m_tcb->m_congState = TcpSocketState::CA_LOSS;
      m_tcb->m_ssThresh = GetSsThresh (m_tcb, BytesInFlight ());
      m_tcb->m_cWnd = m_tcb->m_segmentSize;
    }

  //m_tcb->m_nextTxSequence = m_txBuffer->HeadSequence (); // Restart from highest Ack
  m_tcb->m_nextTxSequence = m_tcb->m_lastAckedSeq; // Restart from highest Ack
  m_dupAckCount = 0;
}

void
ADDCNFlow::NewAck (SequenceNumber32 const& ack, bool resetRTO)
{
  NS_LOG_FUNCTION (this << ack);

  //if (m_state != SYN_RCVD && resetRTO)
  if (resetRTO)
    { // Set RTO unless the ACK is received in SYN_RCVD state
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      // On receiving a "New" ack we restart retransmission timer .. RFC 6298
      // RFC 6298, clause 2.4
      m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

      NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &ADDCNFlow::ReTxTimeout, this);
    }

  // Note the highest ACK and tell app to send more
  NS_LOG_LOGIC (this << " TCP " << this << " NewAck " << ack <<
               " numberAck " << (ack - m_tcb->m_lastAckedSeq)); // Number bytes ack'ed
  /*m_txBuffer->DiscardUpTo (ack);
  if (GetTxAvailable () > 0)
    {
      NotifySend (GetTxAvailable ());
    }
  */
  if (ack > m_tcb->m_nextTxSequence)
    {
      m_tcb->m_nextTxSequence = ack; // If advanced
    }

  if (ack > m_tcb->m_lastAckedSeq)
    {
      m_tcb->m_lastAckedSeq = ack;
    }

  /*
  if (m_txBuffer->Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
    { // No retransmit timer if no data to retransmit
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
    }
  */
}

void
ADDCNFlow::DupAck ()
{
  NS_LOG_FUNCTION (this);
  ++m_dupAckCount;

  if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
    {
      // From Open we go Disorder
      NS_ASSERT_MSG (m_dupAckCount == 1, "From OPEN->DISORDER but with " <<
                     m_dupAckCount << " dup ACKs");

      //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_DISORDER);
      m_tcb->m_congState = TcpSocketState::CA_DISORDER;

      NS_LOG_DEBUG ("OPEN -> DISORDER");
    }

  if (m_tcb->m_congState == TcpSocketState::CA_DISORDER)
    {
      //if ((m_dupAckCount == m_retxThresh) && (m_highRxAckMark >= m_recover))
      if (m_dupAckCount == 3)
        {
          // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
          NS_LOG_DEBUG (TcpSocketState::TcpCongStateName[m_tcb->m_congState] <<
                        " -> RECOVERY");
          // TODO: FastRetransmit ();
          m_recover = m_tcb->m_highTxMark;
          //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_RECOVERY);
          m_tcb->m_congState = TcpSocketState::CA_RECOVERY;

          //m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, BytesInFlight ());
          m_tcb->m_ssThresh = GetSsThresh (m_tcb, BytesInFlight ());
          m_tcb->m_cWnd = m_tcb->m_ssThresh + m_dupAckCount * m_tcb->m_segmentSize;

          NS_LOG_INFO (m_dupAckCount << " dupack. Enter fast recovery mode." <<
               "Reset cwnd to " << m_tcb->m_cWnd << ", ssthresh to " <<
               m_tcb->m_ssThresh << " at fast recovery seqnum " << m_recover);
        }
      //else if (m_limitedTx && m_txBuffer->SizeFromSequence (m_tcb->m_nextTxSequence) > 0)
      //  {
          // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      //    LimitedTransmit ();
      //  }
    }
  else if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_tcb->m_cWnd += m_tcb->m_segmentSize;
      NS_LOG_INFO (m_dupAckCount << " Dupack received in fast recovery mode."
                   "Increase cwnd to " << m_tcb->m_cWnd);
      //SendPendingData (m_connected);
    }

  // Artificially call PktsAcked. After all, one segment has been ACKed.
  // m_congestionControl->PktsAcked (m_tcb, 1, m_lastRtt);
}

void 
ADDCNFlow::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  NS_ASSERT (0 != (tcpHeader.GetFlags () & TcpHeader::ACK));
  NS_ASSERT (m_tcb->m_segmentSize > 0);

  SequenceNumber32 ackNumber = tcpHeader.GetAckNumber ();

  NS_LOG_DEBUG ("ADDCNFlow::ACK of " << ackNumber <<
                " SND.UNA=" << m_tcb->m_lastAckedSeq <<
                " SND.NXT=" << m_tcb->m_nextTxSequence);

  //m_tcb->m_lastAckedSeq = ackNumber;

  if (ackNumber == m_tcb->m_lastAckedSeq
      && ackNumber < m_tcb->m_nextTxSequence
      && (packet->GetSize () - tcpHeader.GetSerializedSize ()) == 0)
    {
      // There is a DupAck
      DupAck ();
    }
  else if (ackNumber == m_tcb->m_lastAckedSeq
           && ackNumber == m_tcb->m_nextTxSequence)
    {
      // Dupack, but the ACK is precisely equal to the nextTxSequence
    }
  else if (ackNumber > m_tcb->m_lastAckedSeq)
    { // Case 3: New ACK, reset m_dupAckCount and update m_txBuffer
      bool callCongestionControl = true;
      bool resetRTO = true;
      uint32_t bytesAcked = ackNumber - m_tcb->m_lastAckedSeq;
      uint32_t segsAcked  = bytesAcked / m_tcb->m_segmentSize;
      m_bytesAckedNotProcessed += bytesAcked % m_tcb->m_segmentSize;

      if (m_bytesAckedNotProcessed >= m_tcb->m_segmentSize)
        {
          segsAcked += 1;
          m_bytesAckedNotProcessed -= m_tcb->m_segmentSize;
        }

      NS_LOG_LOGIC (" Bytes acked: " << bytesAcked <<
                    " Segments acked: " << segsAcked <<
                    " bytes left: " << m_bytesAckedNotProcessed);

      if ((m_ecnState & TcpSocket::ECN_CONN) && ((tcpHeader.GetFlags () & (TcpHeader::SYN | TcpHeader::ECE)) == TcpHeader::ECE))
        {
          NS_LOG_INFO ("Received ECN Echo. Checking if it's a valid one.");
          if (m_ecnEchoSeq < tcpHeader.GetAckNumber ())
            {
              NS_LOG_INFO ("Received ECN Echo was valid.");
              m_ecnEchoSeq = tcpHeader.GetAckNumber ();
              m_ecnState |= TcpSocket::ECN_RX_ECHO;
            }
        }

      /* The following switch is made because m_dupAckCount can be
       * "inflated" through out-of-order segments (e.g. from retransmission,
       * while segments have not been lost but are network-reordered). At
       * least one segment has been acked; in the luckiest case, an amount
       * equals to segsAcked-m_dupAckCount has not been processed.
       *
       * To be clear: segsAcked will be passed to PktsAcked, and it should take
       * in considerations the times that it has been already called, while newSegsAcked
       * will be passed to IncreaseCwnd, and it represents the amount of
       * segments that are allowed to increase the cWnd value.
       */
      uint32_t newSegsAcked = segsAcked;
      if (segsAcked > m_dupAckCount)
        {
          segsAcked -= m_dupAckCount;
        }
      else
        {
          segsAcked = 1;
        }

      if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
        {
          //m_congestionControl->PktsAcked (m_tcb, segsAcked, m_lastRtt);
        }
      else if (m_tcb->m_congState == TcpSocketState::CA_DISORDER)
        {
          // The network reorder packets. Linux changes the counting lost
          // packet algorithm from FACK to NewReno. We simply go back in Open.
          //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
          m_tcb->m_congState = TcpSocketState::CA_OPEN;
          //m_congestionControl->PktsAcked (m_tcb, segsAcked, m_lastRtt);
          m_dupAckCount = 0;
          m_retransOut = 0;

          NS_LOG_DEBUG ("DISORDER -> OPEN");
        }
      else if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
        {
          if (ackNumber < m_recover)
            {
              /* Partial ACK.
               * In case of partial ACK, retransmit the first unacknowledged
               * segment. Deflate the congestion window by the amount of new
               * data acknowledged by the Cumulative Acknowledgment field.
               * If the partial ACK acknowledges at least one SMSS of new data,
               * then add back SMSS bytes to the congestion window.
               * This artificially inflates the congestion window in order to
               * reflect the additional segment that has left the network.
               * Send a new segment if permitted by the new value of cwnd.
               * This "partial window deflation" attempts to ensure that, when
               * fast recovery eventually ends, approximately ssthresh amount
               * of data will be outstanding in the network.  Do not exit the
               * fast recovery procedure (i.e., if any duplicate ACKs subsequently
               * arrive, execute step 4 of Section 3.2 of [RFC5681]).
                */
              m_tcb->m_cWnd = SafeSubtraction (m_tcb->m_cWnd, bytesAcked);

              if (segsAcked >= 1)
                {
                  m_tcb->m_cWnd += m_tcb->m_segmentSize;
                }
              
              //m_tcb->m_cWnd = m_tcb->m_cWnd.Get() > m_tcb->m_segmentSize ? m_tcb->m_cWnd : m_tcb->m_segmentSize;
              m_tcb->m_cWnd = std::max(m_tcb->m_cWnd.Get(), m_tcb->m_segmentSize);

              callCongestionControl = false; // No congestion control on cWnd should be invoked
              m_dupAckCount = SafeSubtraction (m_dupAckCount, segsAcked); // Update the dupAckCount
              m_retransOut  = SafeSubtraction (m_retransOut, 1);  // at least one retransmission
                                                                  // has reached the other side
              //m_txBuffer->DiscardUpTo (ackNumber);  //Bug 1850:  retransmit before newack
              //DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet

              if (m_isFirstPartialAck)
                {
                  m_isFirstPartialAck = false;
                }
              else
                {
                  resetRTO = false;
                }

              /* This partial ACK acknowledge the fact that one segment has been
               * previously lost and now successfully received. All others have
               * been processed when they come under the form of dupACKs
               */
              //m_congestionControl->PktsAcked (m_tcb, 1, m_lastRtt);

              NS_LOG_INFO ("Partial ACK for seq " << ackNumber <<
                           " in fast recovery: cwnd set to " << m_tcb->m_cWnd <<
                           " recover seq: " << m_recover <<
                           " dupAck count: " << m_dupAckCount);
            }
          else if (ackNumber >= m_recover)
            { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
              m_tcb->m_cWnd = std::min (m_tcb->m_ssThresh.Get (),
                                        BytesInFlight () + m_tcb->m_segmentSize);
              m_isFirstPartialAck = true;
              m_dupAckCount = 0;
              m_retransOut = 0;

              /* This FULL ACK acknowledge the fact that one segment has been
               * previously lost and now successfully received. All others have
               * been processed when they come under the form of dupACKs,
               * except the (maybe) new ACKs which come from a new window
               */
              //m_congestionControl->PktsAcked (m_tcb, segsAcked, m_lastRtt);
              newSegsAcked = (ackNumber - m_recover) / m_tcb->m_segmentSize;
              //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
              m_tcb->m_congState = TcpSocketState::CA_OPEN;

              NS_LOG_INFO ("Received full ACK for seq " << ackNumber <<
                           ". Leaving fast recovery with cwnd set to " << m_tcb->m_cWnd);
              NS_LOG_DEBUG ("RECOVERY -> OPEN");
            }
        }
      else if (m_tcb->m_congState == TcpSocketState::CA_LOSS)
        {
          // Go back in OPEN state
          m_isFirstPartialAck = true;
          //m_congestionControl->PktsAcked (m_tcb, segsAcked, m_lastRtt);
          m_dupAckCount = 0;
          m_retransOut = 0;
          if(ackNumber >= m_recover + 1)
            {
              //m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
              m_tcb->m_congState = TcpSocketState::CA_OPEN;
              NS_LOG_DEBUG ("LOSS -> OPEN");
            }
        }

      if (callCongestionControl)
        {
          IncreaseWindow (m_tcb, newSegsAcked);

          NS_LOG_LOGIC ("Congestion control called: " <<
                        " cWnd: " << m_tcb->m_cWnd <<
                        " ssTh: " << m_tcb->m_ssThresh);
        }

      // Reset the data retransmission count. We got a new ACK!
      //m_dataRetrCount = m_dataRetries;

      if (m_isFirstPartialAck == false)
        {
          NS_ASSERT (m_tcb->m_congState == TcpSocketState::CA_RECOVERY);
        }

      NewAck (ackNumber, resetRTO);

      // Try to send more data
      /*
      if (!m_sendPendingDataEvent.IsRunning ())
        {
          m_sendPendingDataEvent = Simulator::Schedule (TimeStep (1),
                                                        &ADDCNFlow::SendPendingData,
                                                        this, m_connected);
        }
      */
    }

  // If there is any data piggybacked, store it into m_rxBuffer
  //if (packet->GetSize () > 0)
  //  {
  //    ReceivedData (packet, tcpHeader);
  //  }
}

void
ADDCNFlow::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }
}

uint32_t
ADDCNFlow::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

void
ADDCNFlow::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}

uint32_t
ADDCNFlow::SafeSubtraction (uint32_t a, uint32_t b)
{
  if (a > b)
    {
      return a-b;
    }

  return 0;
}

void
ADDCNFlow::HalveCwnd ()
{
  NS_LOG_FUNCTION (this);
  m_tcb->m_ssThresh = GetSsThresh (m_tcb, BytesInFlight ());
  //m_tcb->m_ssThresh = std::max ((uint32_t)((1 - m_alpha / 2.0) * Window ()), 2 * m_tcb->m_segmentSize);
  // halve cwnd according to DCTCP algo
  m_tcb->m_cWnd = std::max ((uint32_t)((1 - m_alpha / 2.0) * m_tcb->m_cWnd), m_tcb->m_segmentSize);
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
