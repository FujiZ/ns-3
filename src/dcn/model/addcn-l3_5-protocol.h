// DCN - ADCN Protocol class
// Fuji Z, Winter 2016
#ifndef ADCN_L3_5_PROTOCOL_H
#define ADCN_L3_5_PROTOCOL_H

#include <map>
#include <stdint.h>

#include "ip-l3_5-protocol.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief ad-dcn protocol class.
 *
 * This class implement the l3.5 protocol which provides
 * enables taking control over upper layer congestion
 * control transparently.
 */
class ADCNL3_5Protocol : public IpL3_5Protocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ADCNL3_5Protocol ();
  virtual ~ADCNL3_5Protocol ();

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

private:

  /**
   * @brief GetPacketSize
   * @param packet
   * @param protocol the protocol number of packet
   * @return packet size in bytes
   * Get the data field size of a packet
   */
  uint32_t GetPacketSize (Ptr<Packet> packet, uint8_t protocol);

};

} //namespace dcn
} //namespace ns3

#endif // ADCN_L3_5_PROTOCOL_H
