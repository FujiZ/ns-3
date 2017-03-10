#ifndef C3_DSDIVISION_H
#define C3_DSDIVISION_H

#include "c3-division.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 DS division
 */
class C3DsDivision : public C3Division
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3DsDivision ();

  virtual ~C3DsDivision ();

  //inherited from c3 division
  virtual Ptr<C3Tunnel> GetTunnel (const Ipv4Address &src, const Ipv4Address &dst);
};

} //namespace dcn
} //namespace ns3

#endif // C3_DS_DIVISION_H
