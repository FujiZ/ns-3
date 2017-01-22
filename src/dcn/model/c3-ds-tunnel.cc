#include "c3-ds-tunnel.h"

#include <algorithm>

#include "ns3/flow-id-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3DsTunnel");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3DsTunnel);

TypeId
C3DsTunnel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3DsTunnel")
      .SetParent<C3Tunnel> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3DsTunnel> ()
  ;
  return tid;
}

C3DsTunnel::C3DsTunnel ()
{
  NS_LOG_FUNCTION (this);
}

C3DsTunnel::~C3DsTunnel ()
{
  NS_LOG_FUNCTION (this);
}

uint64_t
C3DsTunnel::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  uint64_t totalBitRate;
  for (auto it = m_flowMap.begin (); it != m_flowMap.end (); ++it)
    {
      totalBitRate += it->second->UpdateRateRequest ();
    }
  m_rateRequest = totalBitRate;
  return m_rateRequest;
}

void
C3DsTunnel::SetRateResponse (uint64_t rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rateResponse = rate;
  if (m_rateResponse > m_rateRequest)
    {
      // all request can be satisfied
      NS_LOG_LOGIC (this << "all request satisfied");
      /// factor that multi by request Rate
      double factor = (double)m_rateResponse / m_rateRequest;
      for (auto it = m_flowMap.begin (); it != m_flowMap.end (); ++it)
        {
          it->second->SetRateResponse (factor * it->second->GetRateRequest ());
        }
    }
  else
    {
      NS_LOG_LOGIC (this << "in-tunnel scheduling");
      //call in-tunnel scheduling
      Schedule ();
    }
}

void
C3DsTunnel::Send (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  FlowIdTag flowIdTag;
  NS_ASSERT (p->FindFirstMatchingByteTag (flowIdTag));
  uint32_t flowId = flowIdTag.GetFlowId ();
  auto iter = m_flowMap.find (flowId);
  if (iter == m_flowMap.end ())
    {
      NS_LOG_LOGIC ("Alloc new flow: " << flowId);
      Ptr<C3DsFlow> newFlow = CreateObject<C3DsFlow> ();
      newFlow->SetForwardTarget (MakeCallback (&C3DsTunnel::Forward, this));
      m_flowMap[flowId] = newFlow;
      m_flowVector.push_back (newFlow);
      iter = m_flowMap.find (flowId);
    }
  NS_ASSERT (iter != m_flowMap.end ());
  iter->second->Send (p->Copy ());
}

void
C3DsTunnel::Schedule (void)
{
  //sort by requestRate
  auto cmp = [] (const Ptr<C3DsFlow> &a, const Ptr<C3DsFlow> &b)
    {
      return a->GetRateRequest () < b->GetRateRequest ();
    };
  std::sort (m_flowVector.begin (), m_flowVector.end (), cmp);
  uint64_t remainRate = m_rateResponse;
  for (auto it = m_flowVector.begin (); it != m_flowVector.end(); ++it)
    {
      uint64_t allocRate = std::min (remainRate, (*it)->GetRateRequest ());
      (*it)->SetRateResponse (allocRate);
      remainRate -= allocRate;
    }
}

} //namespace dcn
} //namespace ns3
