#include "c3-ls-tunnel.h"

#include <algorithm>
#include <vector>

#include "ns3/flow-id-tag.h"
#include "ns3/log.h"

#include "c3-ls-flow.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3LsTunnel");

namespace c3p {

NS_OBJECT_ENSURE_REGISTERED (C3LsTunnel);

TypeId
C3LsTunnel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::c3p::C3LsTunnel")
      .SetParent<C3Tunnel> ()
      .SetGroupName ("C3p")
  ;
  return tid;
}

C3LsTunnel::C3LsTunnel (uint32_t tenantId, const Ipv4Address &src, const Ipv4Address &dst)
  : C3Tunnel (tenantId, C3Type::LS, src, dst)
{
  NS_LOG_FUNCTION (this);
}

C3LsTunnel::~C3LsTunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3LsTunnel::Send (Ptr<Packet> packet, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << packet << static_cast<uint32_t> (protocol));
  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  uint32_t flowId = flowIdTag.GetFlowId ();

  GetFlow (flowId, protocol)->Send (packet);
}

void
C3LsTunnel::ScheduleFlow (void)
{
  NS_LOG_FUNCTION (this);
  int size = m_flowList.size();
  if (size <= 0) return;
  uint64_t avg_rate = GetRate().GetBitRate () / size;
  for (auto it = m_flowList.begin (); it != m_flowList.end (); it++)
  {
    Ptr<C3Flow> flow = it->second;
    flow->SetRate (DataRate (avg_rate));
  }
}

Ptr<C3Flow>
C3LsTunnel::GetFlow (uint32_t fid, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << fid);

  auto it = m_flowList.find (fid);
  if (it != m_flowList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3LsFlow> flow = CreateObject<C3LsFlow> ();
      flow->SetForwardTarget (MakeCallback (&C3LsTunnel::Forward, this));
      flow->SetProtocol (protocol);
      m_flowList[fid] = flow;
      return flow;
    }
}

} //namespace c3p
} //namespace ns3
