// DCN - Layer 3.5 Protocol base class
// Fuji Z, Winter 2016
#ifndef IP_L3_5PROTOCOL_H
#define IP_L3_5PROTOCOL_H

#include "ns3/ip-l4-protocol.h"

namespace ns3 {

class IpL4Protocol;

/**
 * \ingroup dcn
 *
 * \brief L3.5 Protocol abstract base class.
 *
 * This is an abstract base class for layer 3.5 protocols between
 * transport layer and network layer
 */
class IpL3_5Protocol : public IpL4Protocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual ~IpL3_5Protocol ();
};

}


#endif // IP_L3_5_PROTOCOL_H
