#include "ns3/log.h"

#include "rate-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RateControlConnector");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (RateController);

TypeId
RateController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::RateController")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddTraceSource ("RateRequest",
                       "Current rate request",
                       MakeTraceSourceAccessor (&RateController::m_rateRequest),
                       "ns3::DateRate::TracedValueCallback")
      .AddTraceSource ("RateResponse",
                       "rate response from outer layer",
                       MakeTraceSourceAccessor (&RateController::m_rateResponse),
                       "ns3::DateRate::TracedValueCallback")
  ;
  return tid;
}

RateController::~RateController (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

DataRate
RateController::GetRateRequest (void) const
{
  return m_rateRequest;
}

} //namespace dcn
} //namespace ns3
