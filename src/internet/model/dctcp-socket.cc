#include "dctcp-socket.h"

#include "ns3/log.h"

#include "tcp-congestion-ops.h"
#include "tcp-option-ts.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DctcpSocket");

NS_OBJECT_ENSURE_REGISTERED (DctcpSocket);

TypeId
DctcpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DctcpSocket")
      .SetParent<TcpSocketBase> ()
      .SetGroupName ("Internet")
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
    m_ceTransition (false),
    m_ecnTransition (false)
{
}

DctcpSocket::DctcpSocket (const DctcpSocket &sock)
  : TcpSocketBase (sock),
    m_g (sock.m_g),
    m_alpha (sock.m_alpha),
    m_ackedBytesEcn (sock.m_ackedBytesEcn),
    m_ackedBytesTotal (sock.m_ackedBytesTotal),
    m_alphaUpdateSeq (sock.m_alphaUpdateSeq),
    m_dctcpMaxSeq (sock.m_dctcpMaxSeq),
    m_ceTransition (sock.m_ceTransition),
    m_ecnTransition (sock.m_ecnTransition)
{
}

void
DctcpSocket::DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                          const Address &toAddress)
{
  // in case the packet still has a priority tag attached, remove it
  SocketPriorityTag priorityTag;
  packet->RemovePacketTag (priorityTag);

  // Peel off TCP header and do validity checking
  TcpHeader tcpHeader;
  uint32_t bytesRemoved = packet->RemoveHeader (tcpHeader);
  SequenceNumber32 seq = tcpHeader.GetSequenceNumber ();
  if (bytesRemoved == 0 || bytesRemoved > 60)
    {
      NS_LOG_ERROR ("Bytes removed: " << bytesRemoved << " invalid");
      return; // Discard invalid packet
    }

  if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST) && m_ecnReceiverState != ECN_DISABLED)
    {
      // ce transition to support delayed ack
      if (m_ecnReceiverState == ECN_CE_RCVD)
        {
          // received CE in this packet
          m_ecnReceiverState = ECN_IDLE;
          if (!m_ceTransition)
            {
              m_ceTransition = true;
              // send immediate ACK with ECN = 0
              m_ecnTransition = false;
              m_delAckCount = m_delAckMaxCount;
            }
          else
            {
              m_ecnTransition = true;
            }
        }
      else
        {
          if (m_ceTransition)
            {
              m_ceTransition = false;
              // send immediate ACK with ECN = 1
              m_ecnTransition = true;
              m_delAckCount = m_delAckMaxCount;
            }
          else
            {
              m_ecnTransition = false;
            }
        }
    }

  if (packet->GetSize () > 0 && OutOfRange (seq, seq + packet->GetSize ()))
    {
      // Discard fully out of range data packets
      NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                    " received packet of seq [" << seq <<
                    ":" << seq + packet->GetSize () <<
                    ") out of range [" << m_rxBuffer->NextRxSequence () << ":" <<
                    m_rxBuffer->MaxRxSequence () << ")");
      // Acknowledgement should be sent for all unacceptable packets (RFC793, p.69)
      if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
        {
          SendAckPacket ();
        }
      return;
    }

  m_rxTrace (packet, tcpHeader, this);

  if (tcpHeader.GetFlags () & TcpHeader::SYN)
    {
      /* The window field in a segment where the SYN bit is set (i.e., a <SYN>
       * or <SYN,ACK>) MUST NOT be scaled (from RFC 7323 page 9). But should be
       * saved anyway..
       */
      m_rWnd = tcpHeader.GetWindowSize ();

      if (tcpHeader.HasOption (TcpOption::WINSCALE) && m_winScalingEnabled)
        {
          ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
        }
      else
        {
          m_winScalingEnabled = false;
        }

      // When receiving a <SYN> or <SYN-ACK> we should adapt TS to the other end
      if (tcpHeader.HasOption (TcpOption::TS) && m_timestampEnabled)
        {
          ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
                                  tcpHeader.GetSequenceNumber ());
        }
      else
        {
          m_timestampEnabled = false;
        }

      // Initialize cWnd and ssThresh
      m_tcb->m_cWnd = GetInitialCwnd () * GetSegSize ();
      m_tcb->m_ssThresh = GetInitialSSThresh ();

      if (tcpHeader.GetFlags () & TcpHeader::ACK)
        {
          EstimateRtt (tcpHeader);
          m_highRxAckMark = tcpHeader.GetAckNumber ();
        }
    }
  else if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      NS_ASSERT (!(tcpHeader.GetFlags () & TcpHeader::SYN));
      if (m_timestampEnabled)
        {
          if (!tcpHeader.HasOption (TcpOption::TS))
            {
              // Ignoring segment without TS, RFC 7323
              NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                            " received packet of seq [" << seq <<
                            ":" << seq + packet->GetSize () <<
                            ") without TS option. Silently discard it");
              return;
            }
          else
            {
              ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
                                      tcpHeader.GetSequenceNumber ());
            }
        }

      EstimateRtt (tcpHeader);
      UpdateWindowSize (tcpHeader);
    }


  if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
    { // Zero window: Enter persist state to send 1 byte to probe
      NS_LOG_LOGIC (this << " Enter zerowindow persist state");
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_persistTimeout).GetSeconds ());
      m_persistEvent = Simulator::Schedule (m_persistTimeout, &DctcpSocket::PersistTimeout, this);
      NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
    }

  // TCP state machine code in different process functions
  // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
  switch (m_state)
    {
    case ESTABLISHED:
      ProcessEstablished (packet, tcpHeader);
      break;
    case LISTEN:
      ProcessListen (packet, tcpHeader, fromAddress, toAddress);
      break;
    case TIME_WAIT:
      // Do nothing
      break;
    case CLOSED:
      // Send RST if the incoming packet is not a RST
      if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
        { // Since m_endPoint is not configured yet, we cannot use SendRST here
          TcpHeader h;
          Ptr<Packet> p = Create<Packet> ();
          // Send a packet tag for setting ECT bits in IP header
          if (m_ecnReceiverState != ECN_DISABLED)
            {
              SocketIpTosTag ipTosTag;
              ipTosTag.SetTos (0x02);
              p->AddPacketTag (ipTosTag);
              SocketIpv6TclassTag ipTclassTag;
              ipTclassTag.SetTclass (0x02);
              p->AddPacketTag (ipTclassTag);
            }
          h.SetFlags (TcpHeader::RST);
          h.SetSequenceNumber (m_tcb->m_nextTxSequence);
          h.SetAckNumber (m_rxBuffer->NextRxSequence ());
          h.SetSourcePort (tcpHeader.GetDestinationPort ());
          h.SetDestinationPort (tcpHeader.GetSourcePort ());
          h.SetWindowSize (AdvertisedWindowSize ());
          AddOptions (h);
          m_txTrace (p, h, this);
          m_tcp->SendPacket (p, h, toAddress, fromAddress, m_boundnetdevice);
        }
      break;
    case SYN_SENT:
      ProcessSynSent (packet, tcpHeader);
      break;
    case SYN_RCVD:
      ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
      break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
      ProcessWait (packet, tcpHeader);
      break;
    case CLOSING:
      ProcessClosing (packet, tcpHeader);
      break;
    case LAST_ACK:
      ProcessLastAck (packet, tcpHeader);
      break;
    default: // mute compiler
      break;
    }

  if (m_rWnd.Get () != 0 && m_persistEvent.IsRunning ())
    { // persist probes end, the other end has increased the window
      NS_ASSERT (m_connected);
      NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
      m_persistEvent.Cancel ();

      // Try to send more data, since window has been updated
      if (!m_sendPendingDataEvent.IsRunning ())
        {
          m_sendPendingDataEvent = Simulator::Schedule (TimeStep (1),
                                                        &DctcpSocket::SendPendingData,
                                                        this, m_connected);
        }
    }
}

uint32_t
DctcpSocket::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
  NS_LOG_FUNCTION (this << seq << maxSize << withAck);

  bool isRetransmission = false;
  if (seq != m_tcb->m_highTxMark)
    {
      isRetransmission = true;
    }

  Ptr<Packet> p = m_txBuffer->CopyFromSequence (maxSize, seq);
  uint32_t sz = p->GetSize (); // Size of packet
  uint8_t flags = withAck ? TcpHeader::ACK : 0;
  uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));

  if (withAck)
    {
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
    }

  // Sender should reduce the Congestion Window as a response to receiver's ECN Echo notification only once per window
  if (m_ecnSenderState ==  ECN_ECE_RCVD && m_ecnEchoSeq.Get() > m_ecnCWRSeq.Get())
    {
      NS_LOG_INFO ("Backoff mechanism by reducing CWND according to DCTCP algo");
      m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, BytesInFlight ());
      // cut down cwnd according to DCTCP algo
      m_tcb->m_cWnd = std::max ((uint32_t)(m_tcb->m_cWnd * (1 - m_alpha / 2.0)), m_tcb->m_segmentSize);
      flags |= TcpHeader::CWR;
      m_ecnCWRSeq = seq;
      NS_LOG_DEBUG (EcnStateName[m_ecnSenderState] << " -> ECN_CWR_SENT");
      m_ecnSenderState = ECN_CWR_SENT;
      NS_LOG_INFO ("CWR flags set");
      NS_LOG_DEBUG (TcpSocketState::TcpCongStateName[m_tcb->m_congState] << " -> CA_CWR");
      if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
        {
          m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_CWR);
          m_tcb->m_congState = TcpSocketState::CA_CWR;
        }
    }

  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (GetIpTos ())
    {
      SocketIpTosTag ipTosTag;
      NS_LOG_LOGIC (" ECT bits should not be set on retranmsitted packets ");
      if (m_ecnSenderState != ECN_DISABLED && (GetIpTos () & 0x3) == 0 && !isRetransmission)
        {
          ipTosTag.SetTos (GetIpTos () | 0x2);
        }
      else
        {
          ipTosTag.SetTos (GetIpTos ());
        }
      p->AddPacketTag (ipTosTag);
    }
  else
    {
      if (m_ecnSenderState != ECN_DISABLED && !isRetransmission)
        {
          SocketIpTosTag ipTosTag;
          ipTosTag.SetTos (0x02);
          p->AddPacketTag (ipTosTag);
        }
    }

  if (IsManualIpv6Tclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      if (m_ecnSenderState != ECN_DISABLED && (GetIpv6Tclass () & 0x3) == 0 && !isRetransmission)
        {
          ipTclassTag.SetTclass (GetIpv6Tclass () | 0x2);
        }
      else
        {
          ipTclassTag.SetTclass (GetIpv6Tclass ());
        }
      p->AddPacketTag (ipTclassTag);
    }
  else
    {
      if (m_ecnSenderState != ECN_DISABLED && !isRetransmission)
        {
          SocketIpv6TclassTag ipTclassTag;
          ipTclassTag.SetTclass (0x02);
          p->AddPacketTag (ipTclassTag);
        }
    }

  if (IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  uint8_t priority = GetPriority ();
  if (priority)
    {
      SocketPriorityTag priorityTag;
      priorityTag.SetPriority (priority);
      p->ReplacePacketTag (priorityTag);
    }

  if (m_closeOnEmpty && (remainingData == 0))
    {
      flags |= TcpHeader::FIN;
      if (m_state == ESTABLISHED)
        { // On active close: I am the first one to send FIN
          NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
          m_state = FIN_WAIT_1;
        }
      else if (m_state == CLOSE_WAIT)
        { // On passive close: Peer sent me FIN already
          NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
          m_state = LAST_ACK;
        }
    }
  TcpHeader header;
  header.SetFlags (flags);
  header.SetSequenceNumber (seq);
  header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  if (m_endPoint)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  header.SetWindowSize (AdvertisedWindowSize ());
  AddOptions (header);

  if (m_retxEvent.IsExpired ())
    {
      // Schedules retransmit timeout. If this is a retransmission, double the timer

      if (isRetransmission)
        { // This is a retransmit
          // RFC 6298, clause 2.5
          Time doubledRto = m_rto + m_rto;
          m_rto = Min (doubledRto, Time::FromDouble (60,  Time::S));
        }

      NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds () );
      m_retxEvent = Simulator::Schedule (m_rto, &DctcpSocket::ReTxTimeout, this);
    }

  m_txTrace (p, header, this);

  if (m_endPoint)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
      NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                    remainingData << " via TcpL4Protocol to " <<  m_endPoint->GetPeerAddress () <<
                    ". Header " << header);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
      NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                    remainingData << " via TcpL4Protocol to " <<  m_endPoint6->GetPeerAddress () <<
                    ". Header " << header);
    }

  UpdateRttHistory (seq, sz, isRetransmission);

  // Notify the application of the data being sent unless this is a retransmit
  if (seq + sz > m_tcb->m_highTxMark)
    {
      Simulator::ScheduleNow (&DctcpSocket::NotifyDataSent, this,
                             (seq + sz - m_tcb->m_highTxMark.Get ()));
    }
  // Update highTxMark
  m_tcb->m_highTxMark = std::max (seq + sz, m_tcb->m_highTxMark.Get ());
  return sz;

}

void
DctcpSocket::SendAckPacket (void)
{
  NS_LOG_FUNCTION (this);

  if (m_ecnReceiverState != ECN_DISABLED && m_ecnTransition)
    {
      NS_LOG_INFO ("Sending ECN Echo.");
      SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
      m_ecnTransition = false;
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
  // set dctcp max seq to highest
  if (!isRetransmission)
    {
      m_dctcpMaxSeq = std::max (seq + sz, m_dctcpMaxSeq);
    }
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

} // namespace ns3
