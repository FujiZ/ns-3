#ifndef C3_DS_FLOW_H
#define C3_DS_FLOW_H

#include <cstdint>

#include "c3-flow.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the class for Deadline Sensitive Flow
 */
class C3DsFlow : public C3Flow
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  C3DsFlow ();
  virtual ~C3DsFlow ();

  //inherited from RateController
  virtual uint64_t UpdateRateRequest (void);
  virtual void SetRateResponse (uint64_t rate);
  //inherited from C3Flow
  virtual void Send (Ptr<Packet> packet);
private:
  Time m_deadline;        //!< deadline of current flow
};

} //namespace dcn
} //namespace ns3

#endif // C3_DS_FLOW_H
