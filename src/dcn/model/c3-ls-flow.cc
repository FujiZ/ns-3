#include "c3-ls-flow.h"

#include <algorithm>

#include "ns3/log.h"
#include "ns3/packet.h"

#include "c3-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3LsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3LsFlow);

TypeId
C3LsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3LsFlow")
      .SetParent<C3Flow> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3LsFlow> ();
  ;
  return tid;
}

C3LsFlow::C3LsFlow ()
  : C3Flow ()
{
  NS_LOG_FUNCTION (this);
}

C3LsFlow::~C3LsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
C3LsFlow::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  bool retval = packet->PeekPacketTag (c3Tag);
  NS_ASSERT (retval);
  //m_deadline = c3Tag.GetDeadline ();
  C3Flow::Send (packet);
}

bool
C3LsFlow::IsFinished (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_flowSize == 0) return false;
  return !(m_bufferedBytes > 0 || m_sentBytes < m_flowSize);
}

void
C3LsFlow::UpdateInfo (void)
{
  NS_LOG_FUNCTION (this);
  m_weight = 1;
}

} //namespace dcn
} //namespace ns3
