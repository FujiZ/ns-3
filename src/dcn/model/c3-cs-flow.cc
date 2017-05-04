#include "c3-cs-flow.h"

#include "ns3/log.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3CsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3CsFlow);

TypeId
C3CsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3CsFlow")
      .SetParent<C3Flow> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3CsFlow> ();
  ;
  return tid;
}

C3CsFlow::C3CsFlow ()
  : C3Flow ()
{
  NS_LOG_FUNCTION (this);
}

C3CsFlow::~C3CsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
C3CsFlow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  bool retval = packet->PeekPacketTag (c3Tag);
  NS_ASSERT (retval);
  C3Flow::Send (packet);
}

void
C3CsFlow::UpdateInfo (void)
{
  NS_LOG_FUNCTION (this);
  int32_t remainSize = std::max (m_flowSize - m_sentBytes, m_bufferedBytes);
  m_weight = remainSize > 0 ? 1.0 / remainSize : 0;
}

} //namespace dcn
} //namespace ns3
