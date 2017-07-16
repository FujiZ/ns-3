#include "addcn-cs-flow.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADNCsFlow");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADNCsFlow);

TypeId
ADNCsFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADNCsFlow")
      .SetParent<ADDCNFlow> ()
      .SetGroupName ("DCN")
      .AddConstructor<ADNCsFlow> ()
      .AddAttribute("SizeThresh",
                    "Threshold of sent size for judging large flow",
                    DoubleValue(2048),
                    MakeDoubleAccessor (&ADNCsFlow::m_sizeThresh),
                    MakeDoubleChecker<double> (0.0))
      .AddAttribute("WeightMax",
                    "Max weight a flow can get.",
                    DoubleValue(5e-6),
                    MakeDoubleAccessor (&ADNCsFlow::m_weightMax),
                    MakeDoubleChecker<double> (0.0))
      .AddAttribute("WeightMin",
                    "Min weight a flow can get.",
                    DoubleValue(1e-6),
                    MakeDoubleAccessor (&ADNCsFlow::m_weightMin),
                    MakeDoubleChecker<double> (0.0))
  ;
  return tid;
}

ADNCsFlow::ADNCsFlow ()
  : ADDCNFlow (),
    m_weightMax(2.5),
    m_weightMin(0.125)
{
  NS_LOG_FUNCTION (this);
}

ADNCsFlow::~ADNCsFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
ADNCsFlow::UpdateRequestedWeight ()
{
  NS_LOG_FUNCTION (this);

  if (m_sentSize < m_sizeThresh)
    m_weightRequest =  m_weightMax;
  else
    {
      double adder = exp(m_sizeThresh - m_sentSize) * (m_weightMax - m_weightMin);
      m_weightRequest = m_weightMin + adder;
    }
}
}
}
