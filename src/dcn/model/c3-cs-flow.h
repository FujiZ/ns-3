#ifndef C3_CS_FLOW_H
#define C3_CS_FLOW_H

#include <cstdint>

#include "c3-flow.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the class for Deadline Sensitive Flow
 */
class C3CsFlow : public C3Flow
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3CsFlow ();

  virtual ~C3CsFlow ();

  //inherited from C3Flow
  virtual void Send (Ptr<Packet> packet);
  virtual void UpdateInfo (void);

};

} //namespace dcn
} //namespace ns3


#endif // C3_CS_FLOW_H
