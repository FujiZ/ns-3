#ifndef DCTCP_SOCKET_FACTORY_H
#define DCTCP_SOCKET_FACTORY_H

#include "dctcp-socket-factory-base.h"

namespace ns3 {

class DctcpSocketFactory : public DctcpSocketFactoryBase
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

#endif // DCTCP_SOCKET_FACTORY_H
