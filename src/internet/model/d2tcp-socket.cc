#include "d2tcp-socket.h"

#include "ns3/log.h"

#include "ipv4-end-point.h"
#include "ipv6-end-point.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("D2tcpSocket");

NS_OBJECT_ENSURE_REGISTERED (D2tcpSocket);

TypeId
D2tcpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::D2tcpSocket")
      .SetParent<DctcpSocket> ()
      .SetGroupName ("Internet")
      .AddConstructor<D2tcpSocket> ()
      .AddAttribute ("Deadline",
                     "Deadline for current flow.",
                     TimeValue (Time ()),
                     MakeTimeAccessor (&D2tcpSocket::m_deadline),
                     MakeTimeChecker ())
      .AddAttribute ("TotalBytes",
                     "Total bytes tobe sent.",
                     UintegerValue (0),
                     MakeUintegerAccessor (&D2tcpSocket::m_totalBytes),
                     MakeUintegerChecker<uint64_t> ())
      .AddTraceSource ("SentBytes",
                       "Bytes already sent.",
                       MakeTraceSourceAccessor (&D2tcpSocket::m_sentBytes),
                       "ns3::TracedValueCallback::Uint64");
  return tid;
}

TypeId
D2tcpSocket::GetInstanceTypeId () const
{
  return D2tcpSocket::GetTypeId ();
}

D2tcpSocket::D2tcpSocket (void)
  : DctcpSocket (),
    m_deadline (0),
    m_finishTime (0),
    m_totalBytes (0),
    m_sentBytes (0)
{
}

D2tcpSocket::D2tcpSocket (const D2tcpSocket &sock)
  : DctcpSocket (sock),
    m_deadline (sock.m_deadline),
    m_finishTime (sock.m_finishTime),
    m_totalBytes (sock.m_totalBytes),
    m_sentBytes (sock.m_sentBytes)
{
}

int
D2tcpSocket::Connect (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  // If haven't do so, Bind() this socket first
  if (InetSocketAddress::IsMatchingType (address) && m_endPoint6 == 0)
    {
      if (m_endPoint == 0)
        {
          if (Bind () == -1)
            {
              NS_ASSERT (m_endPoint == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint != 0);
        }
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
      SetIpTos (transport.GetTos ());
      m_endPoint6 = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint () != 0)
        {
          NS_LOG_ERROR ("Route to destination does not exist ?!");
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address)  && m_endPoint == 0)
    {
      // If we are operating on a v4-mapped address, translate the address to
      // a v4 address and re-call this function
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address v6Addr = transport.GetIpv6 ();
      if (v6Addr.IsIpv4MappedAddress () == true)
        {
          Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
          return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
        }

      if (m_endPoint6 == 0)
        {
          if (Bind6 () == -1)
            {
              NS_ASSERT (m_endPoint6 == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint6 != 0);
        }
      m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
      m_endPoint = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint6 () != 0)
        { // Route to destination does not exist
          return -1;
        }
    }
  else
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  // Re-initialize parameters in case this socket is being reused after CLOSE
  m_rtt->Reset ();
  m_synCount = m_synRetries;
  m_dataRetrCount = m_dataRetries;

  m_finishTime = m_deadline != Time (0) ? Simulator::Now () + m_deadline : Time (0);

  // DoConnect() will do state-checking and send a SYN packet
  return DoConnect ();
}

void
D2tcpSocket::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz, bool isRetransmission)
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
D2tcpSocket::Fork (void)
{
  return CopyObject<D2tcpSocket> (this);
}

uint32_t
D2tcpSocket::GetSsThresh (void)
{
  NS_LOG_FUNCTION (this);

  double d = 1.0;
  if (m_deadline != Time (0))
    {
      double B = m_totalBytes - m_sentBytes;
      if (B <= 0)
        {
          d = 0.5;
        }
      else
        {
          double Tc = B * m_rtt->GetEstimate ().GetSeconds () / (3.0 * m_tcb->m_cWnd.Get () / 4.0);
          double D = m_finishTime.GetSeconds () - Simulator::Now ().GetSeconds ();
          d = D <= 0 ? 0.5 : std::max (std::min (Tc / D, 2.0), 0.5);
        }
    }
  double p = std::pow (m_alpha, d);
  uint32_t newWnd =  (1 - p / 2.0) * m_tcb->m_cWnd;
  return std::max (newWnd, 2 * m_tcb->m_segmentSize);
}

} // namespace ns3
