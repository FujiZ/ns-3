#ifndef IP_L3_5_PROTOCOL_HELPER_H
#define IP_L3_5_PROTOCOL_HELPER_H

#include <stdint.h>
#include <vector>
#include <string>

#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"

namespace ns3 {
namespace dcn {

class IpL3_5Protocol;

class IpL3_5ProtocolHelper
{
public:
  /**
   * Create an IpL3_5ProtocolHelper that makes life easier for
   * people who want to install L3_5 protocol to nodes.
   */
  IpL3_5ProtocolHelper ();
  IpL3_5ProtocolHelper (const std::string &tid);
  ~IpL3_5ProtocolHelper ();

  /**
   * Get a pointer to the requested aggregated Object.
   * \returns a newly-created L3_5 protocol
   */
  Ptr<IpL3_5Protocol> Create(void) const;

  /**
   * This method creates an l3_5 protocol and install to
   * the given node.
   * should be called after the internet stack is installed
   * \param node The node to install the protocol in
   */
  void Install (Ptr<Node> node) const;

  /**
   * \brief This method creates an l3_5 protocol and install to the given nodes
   * It MUST be called after the internet stack is installed.
   * the downtarget of l3_5 protocol will follow the last
   * l4 protocol added, so it's recommanded to have the same
   * downtarget among the l4 protocols before calling the function.
   * \param nodes The nodes to install the protocol in
   */
  void Install (NodeContainer nodes) const;

  /**
   * Set an attribute value to be propagated to each protocol created by the
   * helper.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   *
   * Set these attribute on each ns3::IpL3_5Protocol created
   * by IpL3_5ProtocolHelper::Install
   */
  void SetAttribute (const std::string &name, const AttributeValue &value);

  void SetIpL3_5Protocol (const std::string &tid);

  void AddIpL4Protocol (const std::string &tid);
  void AddIpL4Protocol (const std::string &tid, uint32_t interface);

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  IpL3_5ProtocolHelper & operator = (const IpL3_5ProtocolHelper &o);

  /**
   * \brief Container of the IPv4 L4 pair: protocol typeid, interface index
   */
  typedef std::pair<TypeId, int32_t> L4ListValue_t;

  /**
   * \brief Container of the IPv4 L4 instances.
   */
  typedef std::vector<L4ListValue_t> L4List_t;

  ObjectFactory m_agentFactory;
  L4List_t m_ipL4Protocols;
};

} //namespace dcn
} //namespace ns3

#endif // IP_L3_5_PROTOCOL_HELPER_H
