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

uint64_t
C3DsFlow::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  //calculate the rate request in bps
  ///\todo if current time >= deadline, free current flow
  ///\todo the calculation of remain size didnt consider retransmission(dont consider in the first version)
  int remainSize = std::max (m_flowSize - m_sendedSize, 0);
  double remainTime = (m_deadline - Simulator::Now ()).ToDouble (Time::S);
  m_rateRequest = remainTime > 0 ? (remainSize << 3) / remainTime : 0;
  return m_rateRequest;
}

void
C3DsFlow::SetRateResponse (uint64_t rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rateResponse = rate;
  m_tbf->SetRate (DataRate (rate));
}

void
C3DsFlow::Send (Ptr<Packet> packet)
{
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_deadline = c3Tag.GetDeadline ();
  C3Flow::Send (packet);
}

} //namespace dcn
} //namespace ns3
