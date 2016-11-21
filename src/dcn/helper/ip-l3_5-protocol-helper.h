#ifndef IP_L3_5_PROTOCOL_HELPER_H
#define IP_L3_5_PROTOCOL_HELPER_H

#include <vector>
#include <string>

#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"

namespace ns3 {

class IpL3_5Protocol;

class IpL3_5ProtocolHelper
{
public:
  /**
   * Create an IpL3_5ProtocolHelper that makes life easier for
   * people who want to install L3_5 protocol to nodes.
   */
  IpL3_5ProtocolHelper ();
  IpL3_5ProtocolHelper (std::string tid);
  ~IpL3_5ProtocolHelper ();

  /**
   * Get a pointer to the requested aggregated Object.
   * \returns a newly-created L3_5 protocol
   */
  Ptr<IpL3_5Protocol> Create() const;

  /**
   * This method creates an l3_5 protocol and install to
   * the given node.
   * should be called after the internet stack is installed
   * \param node The node to install the protocol in
   */
  void Install (Ptr<Node> node) const;

  /**
   * \brief This method creates an l3_5 protocol and install to the given nodes
   * It should be called after the internet stack is installed.
   * the downtarget of l3_5 protocol will follow the last
   * l4 protocol added, so it's recommanded to have the same
   * downtarget among the l4 protocols before calling the function.
   * \param nodes The nodes to install the protocol in
   */
  void Install (NodeContainer nodes) const;

  void SetIpL3_5Protocol (TypeId tid);
  void SetIpL3_5Protocol (std::string tid);

  void AddIpL4Protocol (TypeId tid);
  void AddIpL4Protocol (std::string tid);

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  IpL3_5ProtocolHelper & operator = (const IpL3_5ProtocolHelper &o);
  ObjectFactory m_agentFactory;
  std::vector<TypeId> m_ipL4Protocols;
};

}


#endif // IP_L3_5_PROTOCOL_HELPER_H
