#include "c3-flow.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Flow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Flow);

TypeId
C3Flow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Flow")
      .SetParent<RateController> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3Flow::C3Flow ()
  : m_tbf (CreateObject<TokenBucketFilter> ())
{
  NS_LOG_FUNCTION (this);
}

C3Flow::~C3Flow ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Flow::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  this->m_tbf->SetSendTarget (cb);
}

void
C3Flow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_tbf = 0;
  RateController::DoDispose ();
}

} //namespace dcn
} //namespace ns3
