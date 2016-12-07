#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/uinteger.h"
#include "ns3/drop-tail-queue.h"

#include "token-bucket-filter.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TokenBucketFilter");

NS_OBJECT_ENSURE_REGISTERED (TokenBucketFilter);

TypeId
TokenBucketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TokenBucketFilter")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddAttribute ("DataRate",
                     "The default data rate for TBF in bps",
                     DataRateValue (DataRate ("32768b/s")),
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
{
  NS_LOG_FUNCTION (this);
  m_tokens = bucket;
  m_lastUpdateTime = Simulator::Now ();
  m_queue = GetObject<DropTailQueue> ();
}

TokenBucketFilter::~TokenBucketFilter ()
{
  m_timer.Cancel ();
}

void
TokenBucketFilter::SetSendTarget (SendTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_sendTarget = cb;
}

TokenBucketFilter::SendTargetCallback
TokenBucketFilter::GetSendTarget ()
{
  return m_sendTarget;
}

void
TokenBucketFilter::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  //enque packets appropriately if a non-zero queue already exists
  if (!m_queue->IsEmpty ())
    {
      // leave the drop decision to queue
      m_queue->Enqueue (p);
    }
  else
    {
      UpdateTokens ();
      uint64_t packetSize =  (uint64_t)p->GetSize ()<<3; //packet size in bits
      if (m_tokens >= packetSize)
        {
          m_sendTarget (p);
          m_tokens -= packetSize;
        }
      else
        {
          bool result = m_queue->Enqueue (p);
          if (result)
            {
              ScheduleSend (p);
            }
        }
    }
}

void
TokenBucketFilter::GetUpdatedTokens ()
{
  double now = Simulator.Now ().GetSeconds ();
  m_tokens = std::min (m_bucket,
                       m_tokens+(now-m_lastUpdateTime)*m_rate.GetBitRate ());
  m_lastUpdateTime = Simulator.Now ();
}

void
TokenBucketFilter::Timeout ()
{
  NS_ASSERT (!m_queue->IsEmpty ());
  Ptr<Packet> p = m_queue->Dequeue ()->GetPacket ();
  UpdateTokens ();
  uint64_t packetSize =  (uint64_t)p->GetSize ()<<3; //packet size in bits

  //We simply send the packet here without checking if we have enough tokens
  //since the timer is supposed to fire at the right time
  m_sendTarget (p);
  m_tokens -= packetSize;
  if(!m_queue->IsEmpty ())
    {
      //schedule next event
      ScheduleSend (m_queue->Peek ()->GetPacket ());
    }
}

void
TokenBucketFilter::ScheduleSend (Ptr<Packet> p)
{
  uint64_t packetSize = (uint64_t)p->GetSize ()<<3;
  m_timer.Schedule (Time::FromDouble ((packetSize-m_tokens)/m_rate.GetBitRate (), Time::S);
}

}

