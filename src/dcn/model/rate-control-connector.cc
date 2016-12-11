#include "ns3/log.h"

#include "rate-control-connector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RateControlConnector");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (RateControlConnector);

TypeId
Connector::GetTypeId (void)
{
  ///\todo set trace of rate request/response
  static TypeId tid = TypeId ("ns3::dcn::RateControlConnector")
      .SetParent<Connector> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

RateControlConnector::~RateControlConnector (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

DataRate
RateControlConnector::getRateRequest (void) const
{
  return m_rateRequest;
}

} //namespace dcn
} //namespace ns3
