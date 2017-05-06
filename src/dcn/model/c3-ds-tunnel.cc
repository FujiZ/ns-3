#include "c3-ds-tunnel.h"

#include <algorithm>
#include <vector>

#include "ns3/flow-id-tag.h"
#include "ns3/log.h"

#include "c3-ds-flow.h"

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

C3DsTunnel::C3DsTunnel (uint32_t tenantId, const Ipv4Address &src, const Ipv4Address &dst)
  : C3Tunnel (tenantId, C3Type::DS, src, dst)
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
  NS_LOG_FUNCTION (this << packet << static_cast<uint32_t> (protocol));
  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  uint32_t flowId = flowIdTag.GetFlowId ();

  GetFlow (flowId, protocol)->Send (packet);
}

void
C3DsTunnel::ScheduleFlow (void)
{
  NS_LOG_FUNCTION (this);
  std::vector<Ptr<C3DsFlow> > flowList;
  uint64_t rateRequest = 0;
  for (auto it = m_flowList.begin (); it != m_flowList.end (); ++it)
    {
      Ptr<C3DsFlow> flow = StaticCast<C3DsFlow> (it->second);
      rateRequest += flow->GetRateRequest ().GetBitRate ();
      flowList.push_back (flow);
    }
  if (DataRate (rateRequest) <= GetRate ())
    {
      double factor = static_cast<double> (GetRate ().GetBitRate ()) / rateRequest;
      for (auto flow : flowList)
        {
          flow->SetRate (DataRate (factor * flow->GetRateRequest ().GetBitRate ()));
        }
    }
  else
    {
      auto cmp = [] (const Ptr<C3DsFlow> &a, const Ptr<C3DsFlow> &b)
        {
          return a->GetRateRequest () < b->GetRateRequest ();
        };
      std::sort (flowList.begin (), flowList.end (), cmp);
      uint64_t remainRate = GetRate ().GetBitRate ();
      for (auto it = flowList.begin (); it != flowList.end(); ++it)
        {
          Ptr<C3DsFlow> flow = *it;
          uint64_t allocRate = std::min (remainRate, flow->GetRateRequest ().GetBitRate ());
          flow->SetRate (DataRate (allocRate));
          remainRate -= allocRate;
        }
    }
}

Ptr<C3Flow>
C3DsTunnel::GetFlow (uint32_t fid, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << fid);

  auto it = m_flowList.find (fid);
  if (it != m_flowList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3DsFlow> flow = CreateObject<C3DsFlow> ();
      flow->SetForwardTarget (MakeCallback (&C3DsTunnel::Forward, this));
      flow->SetProtocol (protocol);
      m_flowList[fid] = flow;
      return flow;
    }
}

} //namespace dcn
} //namespace ns3
