#ifndef L2DCT_SOCKET_FACTORY_H
#define L2DCT_SOCKET_FACTORY_H

#include "dctcp-socket-factory-base.h"
#include "tcp-l4-protocol.h"

namespace ns3 {

class L2dctSocketFactory : public DctcpSocketFactoryBase
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual Ptr<Socket> CreateSocket (void);

};

} // namespace ns3

#endif // L2DCT_SOCKET_FACTORY_H
