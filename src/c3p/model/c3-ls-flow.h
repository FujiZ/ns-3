#ifndef C3_LS_FLOW_H
#define C3_LS_FLOW_H

#include <cstdint>

#include "c3-flow.h"

namespace ns3 {
namespace c3p {

/**
 * \ingroup c3p
 * the class for Deadline Sensitive Flow
 */
class C3LsFlow : public C3Flow
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3LsFlow ();

  virtual ~C3LsFlow ();

  //inherited from C3Flow
  virtual void Send (Ptr<Packet> packet);
  virtual void UpdateInfo (void);
  virtual bool IsFinished (void) const;
};

} //namespace c3p
} //namespace ns3

#endif // C3_LS_FLOW_H
