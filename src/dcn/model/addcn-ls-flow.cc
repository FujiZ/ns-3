#include "addcn-ls-flow.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADNLsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADNLsFlow);

TypeId
ADNLsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADNLsFlow")
      .SetParent<ADDCNFlow> ()
      .SetGroupName ("DCN")
      .AddConstructor<ADNLsFlow> ()
  ;
  return tid;
}

ADNLsFlow::ADNLsFlow ()
  : ADDCNFlow ()
{
  NS_LOG_FUNCTION (this);
}

ADNLsFlow::~ADNLsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
ADNLsFlow::UpdateRequestedWeight ()
{
  NS_LOG_FUNCTION (this);
  m_weightRequest = this->GetScaledWeight();
}

bool
ADNLsFlow::IsFinished ()
{
  if (m_flowSize == 0) return false;
  return (m_sentSize >= m_flowSize);
}
}
}
