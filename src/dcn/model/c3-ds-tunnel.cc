#include <algorithm>

#include "ns3/log.h"
#include "ns3/flow-id-tag.h"
#include "ns3/packet.h"

#include "c3-ds-tunnel.h"

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

DataRate
C3DsTunnel::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  uint64_t totalBitRate;
  for (auto it = flowMap.begin (); it != flowMap.end (); ++it)
    {
      totalBitRate += it->second->UpdateRateRequest ();
    }
  m_rateRequest = DataRate (totalBitRate);
  return m_rateRequest;
}

void
C3DsTunnel::SetRateResponse (const DataRate &rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rateResponse = rate;
  if (m_rateResponse > m_rateRequest)
    {
      // all request can be satisfied
      NS_LOG_LOGIC (this << "all request satisfied");
      /// factor that multi by request Rate
      double factor =
          (double)m_rateResponse.Get ().GetBitRate ()/m_rateRequest.GetBitRate ();
      for (auto it = flowMap.begin (); it != flowMap.end (); ++it)
        {
          flow->SetRateResponse (factor * it->second->GetRateRequest ());
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
  auto iter = flowMap.find (flowId);
  if (iter == flowMap.end ())
    {
      NS_LOG_LOGIC ("Alloc new flow: " << flowId);
      Ptr<C3DsFlow> newFlow = CreateObject<C3DsFlow> ();
      flowMap[flowId] = newFlow;
      flowVec.push_back (newFlow);
      iter = flowMap.find (flowId);
    }
  NS_ASSERT (iter != flowMap.end ());
  (*iter->second)->Send (p->Copy ());
}

void
C3DsTunnel::Schedule (void)
{
  //sort by requestRate
  auto cmp = [] (const Ptr<C3DsFlow> &a, const Ptr<C3DsFlow> &b)
    {
      return a->GetRateRequest () < b->GetRateRequest ();
    };
  std::sort (flowVec.begin (), flowVec.end (), cmp);
  uint64_t remainRate = m_rateResponse.Get ().GetBitRate ();
  for (auto it = flowVec.begin (); it != flowVec.end(); ++it)
    {
      uint64_t allocRate = std::min (remainRate, (*it)->GetRateRequest ());
      (*it)->SetRateResponse (DataRate (allocRate));
      remainRate -= allocRate;
    }
}

} //namespace dcn
} //namespace ns3
