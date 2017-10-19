#include "c3-ds-flow.h"

#include <algorithm>

#include "ns3/log.h"
#include "ns3/packet.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3DsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3DsFlow);

TypeId
C3DsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3DsFlow")
      .SetParent<C3Flow> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3DsFlow> ();
  ;
  return tid;
}

C3DsFlow::C3DsFlow ()
  : C3Flow ()
{
  NS_LOG_FUNCTION (this);
}

C3DsFlow::~C3DsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
C3DsFlow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  bool retval = packet->PeekPacketTag (c3Tag);
  NS_ASSERT (retval);
  m_deadline = c3Tag.GetDeadline ();
  C3Flow::Send (packet);
}

void
C3DsFlow::UpdateInfo (void)
{
  NS_LOG_FUNCTION (this);
  int32_t remainSize = std::max (m_flowSize - m_sentBytes, m_bufferedBytes);
  double remainTime = (m_deadline - Simulator::Now ()).GetSeconds ();
  m_rateRequest = DataRate (remainTime > 0 ? (remainSize << 3) / remainTime : 0);
  m_weight = m_rateRequest.GetBitRate ();
}

DataRate
C3DsFlow::GetRateRequest (void) const
{
  return m_rateRequest;
}

} //namespace dcn
} //namespace ns3
