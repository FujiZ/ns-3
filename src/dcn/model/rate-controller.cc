#include "rate-controller.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RateController");

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
                       MakeTraceSourceAccessor (&RateController::m_rateRequest))
      .AddTraceSource ("RateResponse",
                       "rate response from outer layer",
                       MakeTraceSourceAccessor (&RateController::m_rateResponse))
  ;
  return tid;
}

RateController::~RateController (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

uint64_t
RateController::GetRateRequest (void) const
{
  return m_rateRequest;
}

} //namespace dcn
} //namespace ns3
