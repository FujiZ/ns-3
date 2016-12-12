#ifndef C3_DIVISION_H
#define C3_DIVISION_H

#include <map>

#include "rate-control-connector.h"

namespace ns3 {
namespace dcn {

class C3Tunnel;

/**
 * \ingroup dcn
 *
 * \brief c3 division implementation
 */
class C3Division : public RateControlConnector
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Division();

  virtual ~C3Division();
private:
  //std::map<C3Tag, C3Tunnel> tunnels;
  //c3tag, c3dstag, c3cstag etc.
  //std::map<fid_t, C3Tunnel> flows;
};

} //namespace dcn
} //namespace ns3

#endif // C3_DIVISION_H
