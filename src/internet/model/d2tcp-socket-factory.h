#ifndef D2TCP_SOCKET_FACTORY_H
#define D2TCP_SOCKET_FACTORY_H

#include "dctcp-socket-factory-base.h"
#include "tcp-l4-protocol.h"

namespace ns3 {

class D2tcpSocketFactory : public DctcpSocketFactoryBase
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

#endif // D2TCP_SOCKET_FACTORY_H
