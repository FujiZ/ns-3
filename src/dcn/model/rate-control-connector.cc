#include "ns3/log.h"

#include "rate-control-connector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RateControlConnector");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (RateControlConnector);

TypeId
RateControlConnector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::RateControlConnector")
      .SetParent<Connector> ()
      .SetGroupName ("DCN")
      .AddTraceSource ("RateRequest",
                       "Current rate request",
                       MakeTraceSourceAccessor (&RateControlConnector::m_rateRequest),
                       "ns3::DateRate::TracedValueCallback")
      .AddTraceSource ("RateResponse",
                       "rate response from outer layer",
                       MakeTraceSourceAccessor (&RateControlConnector::m_rateResponse),
                       "ns3::DateRate::TracedValueCallback")
  ;
  return tid;
}

RateControlConnector::~RateControlConnector (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

DataRate
RateControlConnector::GetRateRequest (void) const
{
  return m_rateRequest;
}

} //namespace dcn
} //namespace ns3
