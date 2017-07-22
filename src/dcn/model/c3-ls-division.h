#ifndef C3_LSDIVISION_H
#define C3_LSDIVISION_H

#include "c3-division.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 LS division
 */
class C3LsDivision : public C3Division
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3LsDivision ();

  virtual ~C3LsDivision ();

  //inherited from c3 division
  virtual Ptr<C3Tunnel> GetTunnel (const Ipv4Address &src, const Ipv4Address &dst);
};

} //namespace dcn
} //namespace ns3

#endif // C3_LS_DIVISION_H
