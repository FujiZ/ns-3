// DCN - C3 Protocol class
// Fuji Z, Winter 2016
#ifndef C3_L3_5_PROTOCOL_H
#define C3_L3_5_PROTOCOL_H

#include <stdint.h>

#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

#include "ip-l3_5-protocol.h"

namespace ns3 {

class Node;
class Socket;

/**
 * \ingroup dcn
 *
 * \brief c3 protocol class.
 *
 * This class implement the c3 protocol which provides
 * multi-tenant multi-objective bandwidth allocation.
 */
class C3L3_5Protocol : public IpL3_5Protocol
{
public:

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  static const uint8_t PROT_NUMBER; //!< protocol number (0xfd=253)

  C3L3_5Protocol ();
  virtual ~C3L3_5Protocol ();

  /**
   * Set node associated with this stack
   * \param node the node
   */
  void SetNode (Ptr<Node> node);

  virtual int GetProtocolNumber (void) const;

  // inherited from Ipv4L4Protocol
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                               Ipv4Header const &header,
                                               Ptr<Ipv4Interface> incomingInterface);
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                               Ipv6Header const &header,
                                               Ptr<Ipv6Interface> incomingInterface);

private:
  Ptr<Node> m_node; //!< the node this stack is associated with
};

}

#endif // C3_L3_5_PROTOCOL_H
