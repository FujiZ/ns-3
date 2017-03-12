#ifndef C3_CS_DIVISION_H
#define C3_CS_DIVISION_H

#include "c3-division.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 CS division
 */
class C3CsDivision : public C3Division
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3CsDivision ();

  virtual ~C3CsDivision ();

  //inherited from c3 division
  virtual Ptr<C3Tunnel> GetTunnel (const Ipv4Address &src, const Ipv4Address &dst);
};

} //namespace dcn
} //namespace ns3

#endif // C3_CS_DIVISION_H
