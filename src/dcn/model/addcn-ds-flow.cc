#include "addcn-ds-flow.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADNDsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADNDsFlow);

TypeId
ADNDsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADNDsFlow")
      .SetParent<ADDCNFlow> ()
      .SetGroupName ("DCN")
      .AddConstructor<ADNDsFlow> ()
  ;
  return tid;
}

ADNDsFlow::ADNDsFlow ()
  : ADDCNFlow ()
{
  NS_LOG_FUNCTION (this);
}

ADNDsFlow::~ADNDsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
ADNDsFlow::UpdateRequestedWeight ()
{
  NS_LOG_FUNCTION (this);

  int32_t remain_size = m_flowSize - m_sentSize;
  double remain_time = (m_deadline - Simulator::Now()).GetSeconds();
  double ergency = remain_size > 0 ? remain_size / remain_time : 0.0;

  m_weightRequest = ergency * this->GetScaledWeight();
}

bool
ADNDsFlow::IsFinished ()
{
  return (m_sentSize >= m_flowSize || m_deadline <= Simulator::Now());
}
}
}
