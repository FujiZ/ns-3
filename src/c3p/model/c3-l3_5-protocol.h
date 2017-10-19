// C3p - C3 Protocol class
// Fuji Z, Winter 2016
#ifndef C3_L3_5_PROTOCOL_H
#define C3_L3_5_PROTOCOL_H

#include <map>
#include <stdint.h>

#include "ns3/ip-l3_5-protocol.h"

namespace ns3 {
namespace c3p {

/**
 * \ingroup c3p
 *
 * \brief c3 protocol class.
 *
 * This class implement the c3 protocol which provides
 * multi-tenant multi-objective bandwidth allocation.
 */
class C3L3_5Protocol : public dcn::IpL3_5Protocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3L3_5Protocol ();
  virtual ~C3L3_5Protocol ();

  // inherited from IpL3_5Protocol
  virtual void Send (Ptr<Packet> packet,
                     Ipv4Address src, Ipv4Address dst,
                     uint8_t protocol, Ptr<Ipv4Route> route);

  virtual void Send6 (Ptr<Packet> packet,
                      Ipv6Address src, Ipv6Address dst,
                      uint8_t protocol, Ptr<Ipv6Route> route);

  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> packet,
                                               Ipv4Header const &header,
                                               Ptr<Ipv4Interface> incomingInterface);
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> packet,
                                               Ipv6Header const &header,
                                               Ptr<Ipv6Interface> incomingInterface);
protected:

  virtual void DoDispose (void);

};

} //namespace c3p
} //namespace ns3

#endif // C3_L3_5_PROTOCOL_H
