#include "token-bucket-filter.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/uinteger.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TokenBucketFilter");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (TokenBucketFilter);

TypeId
TokenBucketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::TokenBucketFilter")
      .SetParent<Connector> ()
      .SetGroupName ("DCN")
      .AddConstructor<TokenBucketFilter> ()
      .AddAttribute ("DataRate",
                     "The default data rate for TBF in bps",
                     DataRateValue (DataRate ("2048b/s")),
                     MakeDataRateAccessor (&TokenBucketFilter::GetRate,
                                           &TokenBucketFilter::SetRate),
                     MakeDataRateChecker ())
      .AddAttribute ("Bucket",
                     "The initial bucket for TBF",
                     UintegerValue (2048),
                     MakeUintegerAccessor (&TokenBucketFilter::m_bucket),
                     MakeUintegerChecker<uint64_t> ())
      .AddAttribute ("QueueLimit",
                     "Queue limit for tbf",
                     UintegerValue (250),
                     MakeUintegerAccessor (&TokenBucketFilter::SetQueueLimit,
                                           &TokenBucketFilter::GetQueueLimit),
                     MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

TokenBucketFilter::TokenBucketFilter ()
  : m_rate (0),
    m_bucket (0),
    m_tokens (0),
    m_init (true),
    m_timer (Timer::CANCEL_ON_DESTROY)
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Simulator::Now ();
  m_queue = CreateObject<DropTailQueue> ();
  m_queue->SetMode (Queue::QUEUE_MODE_PACKETS);
  m_queue->SetDropCallback (MakeCallback (&TokenBucketFilter::Drop, this));
  m_timer.SetFunction (&TokenBucketFilter::Transmit, this);
}

TokenBucketFilter::~TokenBucketFilter ()
{
  NS_LOG_FUNCTION (this);
}

void
TokenBucketFilter::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_timer.Cancel ();
  // drop all packet in the queue
  while (!m_queue->IsEmpty ())
    {
      m_dropTarget (m_queue->Dequeue ()->GetPacket ());
    }
  Connector::DoDispose ();
}

void
TokenBucketFilter::Send (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  // start with a full bucket
  if (m_init)
    {
      m_tokens = static_cast<double> (m_bucket);
      m_lastUpdateTime = Simulator::Now ();
      m_init = false;
    }

  //enque packets appropriately if a non-zero queue already exists
  if (!m_queue->IsEmpty ())
    {
      // leave the drop decision to queue
      NS_LOG_DEBUG ("Enqueue packet: " << p);
      m_queue->Enqueue (Create<QueueItem> (p));
    }
  else
    {
      UpdateTokens ();
      uint64_t packetSize = static_cast<uint64_t> (p->GetSize ()) << 3; //packet size in bits
      NS_LOG_DEBUG ("tokens: "<< m_tokens << " size: " << packetSize);
      if (m_tokens >= packetSize)
        {
          m_sendTarget (p);
          m_tokens -= packetSize;
        }
      else
        {
          if (m_queue->Enqueue (Create<QueueItem> (p)) && m_rate.GetBitRate ())
            {
              m_timer.Schedule (GetSendDelay (p));
            }
        }
    }
}

DataRate
TokenBucketFilter::GetRate (void) const
{
  return m_rate;
}

void
TokenBucketFilter::SetRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  if (rate == DataRate (0) && !m_queue->IsEmpty ())
    {
      NS_LOG_DEBUG (Simulator::Now () << this << " rate to 0");
    }
  m_rate = rate;
  // 如果当前queue非空 && m_timer无调度任务 && rate非0
  if (m_timer.IsExpired () && !m_queue->IsEmpty () && m_rate.GetBitRate ())
    {
      //schedule next event
      m_timer.Schedule (GetSendDelay (m_queue->Peek ()->GetPacket ()));
    }
}

uint32_t
TokenBucketFilter::GetQueueLimit (void) const
{
  return m_queue->GetMaxPackets ();
}

void
TokenBucketFilter::SetQueueLimit (uint32_t limit)
{
  m_queue->SetMaxPackets (limit);
}

void
TokenBucketFilter::Drop (Ptr<QueueItem> item)
{
  NS_LOG_FUNCTION (this << item->GetPacket ());
  m_dropTarget (item->GetPacket ());
}

void
TokenBucketFilter::UpdateTokens (void)
{
  m_tokens = std::min (static_cast<double> (m_bucket),
                       m_tokens + (Simulator::Now ().GetSeconds () - m_lastUpdateTime.GetSeconds ()) * m_rate.GetBitRate ());
  m_lastUpdateTime = Simulator::Now ();
}

void
TokenBucketFilter::Transmit (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!m_queue->IsEmpty ());
  Ptr<Packet> p = m_queue->Dequeue ()->GetPacket ();
  UpdateTokens ();
  uint64_t packetSize = static_cast<uint64_t> (p->GetSize ()) << 3; //packet size in bits

  //We simply send the packet here without checking if we have enough tokens
  //since the timer is supposed to fire at the right time
  m_sendTarget (p);
  m_tokens -= packetSize;
  if(m_rate.GetBitRate () && !m_queue->IsEmpty ())
    {
      //schedule next event
      m_timer.Schedule (GetSendDelay (m_queue->Peek ()->GetPacket ()));
    }
}

Time
TokenBucketFilter::GetSendDelay (Ptr<const Packet> p) const
{
  uint64_t packetSize = static_cast<uint64_t> (p->GetSize ()) << 3;
  return Time::FromDouble (std::max (packetSize - m_tokens, 0.0) / m_rate.GetBitRate (), Time::S);
}

} //namespace dcn
} //namespace ns3

