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
  ;
  return tid;
}

C3DsTunnel::C3DsTunnel (const Ipv4Address &src, const Ipv4Address &dst)
  : C3Tunnel (src, dst)
{
  NS_LOG_FUNCTION (this);
}

C3DsTunnel::~C3DsTunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3DsTunnel::Send (Ptr<Packet> packet, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << packet << (uint32_t)protocol);
  FlowIdTag flowIdTag;
  NS_ASSERT (packet->PeekPacketTag (flowIdTag));
  uint32_t flowId = flowIdTag.GetFlowId ();

  Ptr<C3Flow> flow = GetFlow (flowId);
  //flow->SetForwardTarget (&C3DsTunnel::Forward, this);
  //flow->SetProtocol(protocol);
  flow->Send (packet);
}

/*
uint64_t
C3DsTunnel::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  uint64_t totalBitRate = 0;
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
*/


void
C3DsTunnel::Schedule (void)
{
  /*
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
    */
}

} //namespace dcn
} //namespace ns3
