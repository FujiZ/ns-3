#include "c3-cs-tunnel.h"

#include <cmath>
#include <vector>

#include "ns3/flow-id-tag.h"
#include "ns3/log.h"

#include "c3-cs-flow.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3CsTunnel");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3CsTunnel);

TypeId
C3CsTunnel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3CsTunnel")
      .SetParent<C3Tunnel> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3CsTunnel::C3CsTunnel (uint32_t tenantId, const Ipv4Address &src, const Ipv4Address &dst)
  : C3Tunnel (tenantId, C3Type::CS, src, dst)
{
  NS_LOG_FUNCTION (this);
}

C3CsTunnel::~C3CsTunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3CsTunnel::Send (Ptr<Packet> packet, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << packet << static_cast<uint32_t> (protocol));

  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  uint32_t flowId = flowIdTag.GetFlowId ();

  GetFlow (flowId, protocol)->Send (packet);
}

void
C3CsTunnel::ScheduleFlow (void)
{
  NS_LOG_FUNCTION (this);

  double weight = std::fabs (GetWeightRequest ()) > 10e-7 ? GetWeightRequest () : 1.0;
  for (auto it = m_flowList.begin (); it != m_flowList.end (); ++it)
    {
      Ptr<C3Flow> flow = it->second;
      flow->SetRate (DataRate (flow->GetWeight () / weight * GetRate ().GetBitRate ()));
    }
}

Ptr<C3Flow>
C3CsTunnel::GetFlow (uint32_t fid, uint8_t protocol)
{
  NS_LOG_FUNCTION (this << fid);

  auto it = m_flowList.find (fid);
  if (it != m_flowList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3CsFlow> flow = CreateObject<C3CsFlow> ();
      flow->SetForwardTarget (MakeCallback (&C3CsTunnel::Forward, this));
      flow->SetProtocol (protocol);
      m_flowList[fid] = flow;
      return flow;
    }
}

} //namespace dcn
} //namespace ns3
