// DCN - C3 Protocol class
// Fuji Z, Winter 2016
#ifndef C3_L3_5_PROTOCOL_H
#define C3_L3_5_PROTOCOL_H

#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ip-l3_5-protocol.h"

namespace ns3 {

class Node;
class Socket;
class IpL3_5Protocol;

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
  static const uint8_t PROT_NUMBER; //!< protocol number (0xfd)

  C3L3_5Protocol ();
  virtual ~C3L3_5Protocol ();

  /**
   * Set node associated with this stack
   * \param node the node
   */
  void SetNode (Ptr<Node> node);

  virtual int GetProtocolNumber (void) const;
};

}

#endif // C3_L3_5_PROTOCOL_H
