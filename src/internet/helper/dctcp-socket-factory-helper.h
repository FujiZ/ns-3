#ifndef DCTCP_SOCKET_FACTORY_HELPER_H
#define DCTCP_SOCKET_FACTORY_HELPER_H

#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/ptr.h>
#include <ns3/type-id.h>

#include <string>
#include <vector>

namespace ns3 {

class DctcpSocketFactoryHelper
{
public:
  virtual ~DctcpSocketFactoryHelper ();

  /**
   * @brief AddSocketFactory
   * @param tid typeId of socket factory
   */
  void AddSocketFactory (const std::string &tid);

  /**
   * @brief Install socket factory to nodes
   * @param nodes
   */
  void Install (NodeContainer nodes);

  void Install (Ptr<Node> node);

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  DctcpSocketFactoryHelper & operator = (const DctcpSocketFactoryHelper &o);

  std::vector <TypeId> m_socketFactorys;
};

}

#endif // DCTCP_SOCKET_FACTORY_HELPER_H
