// DCN - C3 Protocol class
// Fuji Z, Winter 2016
#ifndef C3_L3_5_PROTOCOL_H
#define C3_L3_5_PROTOCOL_H

#include <stdint.h>
#include <map>

//#include "c3-division.h"
#include "ip-l3_5-protocol.h"

namespace ns3 {
namespace dcn {

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
  static TypeId GetTypeId (void);

  C3L3_5Protocol ();
  virtual ~C3L3_5Protocol ();

  // inherited from IpL3_5Protocol
  virtual void Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination,
                     uint8_t protocol, Ptr<Ipv4Route> route);
  virtual void Send6 (Ptr<Packet> packet, Ipv6Address source, Ipv6Address destination,
                      uint8_t protocol, Ptr<Ipv6Route> route);

  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> packet,
                                               Ipv4Header const &header,
                                               Ptr<Ipv4Interface> incomingInterface);
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> packet,
                                               Ipv6Header const &header,
                                               Ptr<Ipv6Interface> incomingInterface);
protected:

  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:

  /**
   * @brief GetPacketSize
   * @param packet
   * @param protocol the protocol number of packet
   * @return packet size in bytes
   * Get the data field size of a packet
   */
  uint32_t GetPacketSize (Ptr<Packet> packet, uint8_t protocol);

private:
  //std::map<Ipv4Address, Ptr<C3Division> > m_divisionMap;    //!< <dst, division>
  //std::map<Ipv4Address, Ptr<C3EcnHandler> > m_ecnHandlerMap;    //!< <dst, handler>

};

} //namespace dcn
} //namespace ns3

#endif // C3_L3_5_PROTOCOL_H
