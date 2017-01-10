#ifndef RATE_CONTROL_CONNECTOR_H
#define RATE_CONTROL_CONNECTOR_H

#include <stdint.h>

#include "ns3/data-rate.h"
#include "ns3/traced-value.h"
#include "ns3/object.h"
#include "ns3/packet.h"


namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief rate controller
 * the base class of Connector wiht rate control
 * which is the base class of division/tunnel/flow(session)
 */
class RateController : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual ~RateController ();
public:
  /**
   * \brief updateRateRequest
   * \return current rate request in bps
   * called by outer layer to update the current rate
   * request.
   */
  virtual uint64_t UpdateRateRequest (void) = 0;
  /**
   * \brief getRateRequest
   * \return current rate request in bps
   * called by outer layer to get the current rate
   * request.
   */
  uint64_t GetRateRequest (void) const;
  /**
   * \brief setRateResponse
   * \param rate the rate response
   * called by outer layer to set the rate allocated to this.
   */
  virtual void SetRateResponse (uint64_t rate) = 0;

protected:
  TracedValue<uint64_t> m_rateRequest;
  TracedValue<uint64_t> m_rateResponse;
};

} //namespace dcn
} //namespace ns3

#endif // RATE_CONTROL_CONNECTOR_H
