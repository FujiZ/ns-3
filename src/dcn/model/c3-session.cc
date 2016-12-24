#include "ns3/log.h"

#include "token-bucket-filter.h"

#include "c3-session.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Session");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Session);

TypeId
C3Session::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Session")
      .SetParent<RateController> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

C3Session::C3Session ():
  m_tbf (CreateObject<TokenBucketFilter> ())
{
  NS_LOG_FUNCTION (this);
}

C3Session::~C3Session ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Session::SetForwardTargetCallback (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this << cb);
  this->m_tbf->SetSendTarget (cb);
}

void
C3Session::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_tbf = 0;
  RateController::DoDispose ();
}

} //namespace dcn
} //namespace ns3
