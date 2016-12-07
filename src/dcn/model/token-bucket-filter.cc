#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/uinteger.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/callback.h"

#include "token-bucket-filter.h"

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
                     MakeDataRateAccessor (&TokenBucketFilter::m_rate),
                     MakeDataRateChecker ())
      .AddAttribute ("Bucket",
                     "The default bucket for TBF",
                     UintegerValue (32768),
                     MakeUintegerAccessor (&TokenBucketFilter::m_bucket),
                     MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

TokenBucketFilter::TokenBucketFilter ()
  : m_queue (CreateObject<DropTailQueue> ()), m_timer (Timer::CANCEL_ON_DESTROY)
{
  NS_LOG_FUNCTION (this);
}

TokenBucketFilter::~TokenBucketFilter ()
{
  NS_LOG_FUNCTION (this);
}

void
TokenBucketFilter::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_tokens = m_bucket;
  m_lastUpdateTime = Simulator::Now ();
  m_queue->SetDropCallback (MakeCallback (&TokenBucketFilter::DropItem, this));
  m_timer.SetFunction (&TokenBucketFilter::Timeout, this);
  Connector::DoInitialize ();
}

void
TokenBucketFilter::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue->Cleanup ();
  m_timer.Cancel ();
  Connector::DoDispose ();
}


void
TokenBucketFilter::DoReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  //enque packets appropriately if a non-zero queue already exists
  if (!m_queue->IsEmpty ())
    {
      // leave the drop decision to queue
      m_queue->Enqueue (Create<QueueItem> (p));
    }
  else
    {
      UpdateTokens ();
      uint64_t packetSize =  (uint64_t)p->GetSize ()<<3; //packet size in bits
      if (m_tokens >= packetSize)
        {
          Send (p);
          m_tokens -= packetSize;
        }
      else
        {
          bool result = m_queue->Enqueue (Create<QueueItem> (p));
          if (result)
            {
              m_timer.Schedule (GetSendDelay (p));
            }
        }
    }
}

void
TokenBucketFilter::DropItem (Ptr<QueueItem> item)
{
  Drop (item->GetPacket ());
}

void
TokenBucketFilter::UpdateTokens (void)
{
  double now = Simulator::Now ().GetSeconds ();
  m_tokens = std::min ((double)m_bucket,
                       m_tokens+(now-m_lastUpdateTime.GetSeconds ())*m_rate.GetBitRate ());
  m_lastUpdateTime = Simulator::Now ();
}

void
TokenBucketFilter::Timeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!m_queue->IsEmpty ());
  Ptr<Packet> p = m_queue->Dequeue ()->GetPacket ();
  UpdateTokens ();
  uint64_t packetSize = (uint64_t)p->GetSize ()<<3; //packet size in bits

  //We simply send the packet here without checking if we have enough tokens
  //since the timer is supposed to fire at the right time
  Send (p);
  m_tokens -= packetSize;
  if(!m_queue->IsEmpty ())
    {
      //schedule next event
      m_timer.Schedule (GetSendDelay (m_queue->Peek ()->GetPacket ()));
    }
}

Time
TokenBucketFilter::GetSendDelay (Ptr<Packet> p)
{
  uint64_t packetSize = (uint64_t)p->GetSize ()<<3;
  return Time::FromDouble ((packetSize-m_tokens)/m_rate.GetBitRate (), Time::S);
}

} //namespace dcn
} //namespace ns3

