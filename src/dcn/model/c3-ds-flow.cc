#include "ns3/log.h"
#include "ns3/packet.h"

#include "token-bucket-filter.h"
#include "c3-tag.h"

#include "c3-ds-flow.h"

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

DataRate
C3DsFlow::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  ///\todo if current time >= deadline, free current flow
  //calculate the rate request in bps
  ///\todo the calculation of remain size didnt consider retransmission
  uint32_t remainSize = m_flowSize - m_sendedSize;
  m_rateRequest = DataRate ((uint64_t)((remainSize << 3)
                                       /m_deadline.ToDouble (Time::S)));
  return m_rateRequest;
}

void
C3DsFlow::SetRateResponse (const DataRate &rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rateResponse = rate;
  m_tbf->SetAttribute ("DataRate", DataRateValue (rate));
}

void
C3DsFlow::Send (Ptr<Packet> p)
{
  C3Tag c3Tag;
  NS_ASSERT (p->FindFirstMatchingByteTag (c3Tag));
  m_deadline = c3Tag.GetDeadline ();
  m_bufferSize += c3Tag.GetPacketSize ();
  m_tbf->Send (p);
}

} //namespace dcn
} //namespace ns3
