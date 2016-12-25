#ifndef C3_DS_FLOW_H
#define C3_DS_FLOW_H

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
  virtual DataRate UpdateRateRequest (void);
  virtual void SetRateResponse (const DataRate &rate);
protected:
  //inherited from C3Flow
  virtual void DoSend (Ptr<Packet> p);
private:
  Time m_deadline;
};

} //namespace dcn
} //namespace ns3

#endif // C3_DS_FLOW_H
