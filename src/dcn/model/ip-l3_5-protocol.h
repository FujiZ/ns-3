// DCN - Layer 3.5 Protocol base class
// Fuji Z, Winter 2016
#ifndef IP_L3_5PROTOCOL_H
#define IP_L3_5PROTOCOL_H

#include <stdint.h>

#include "ns3/ip-l4-protocol.h"
#include "ns3/type-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

namespace ns3 {

class Node;
class Packet;
class Ipv4Route;
class Ipv6Route;
class Ipv4Interface;
class Ipv6Interface;

/**
 * \ingroup dcn
 *
 * \brief L3.5 Protocol abstract base class.
 *
 * This is an abstract base class for layer 3.5 protocols between
 * transport layer and network layer
 */
class IpL3_5Protocol : public IpL4Protocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual ~IpL3_5Protocol ();

  /**
   * Set node associated with this stack
   * \param node the node
   */
  void SetNode (Ptr<Node> node);

  /**
   * \brief This function is called by higher layer protocol when sending packets
   */
  virtual void Send (Ptr<Packet> packet, Ipv4Address source,
                     Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route) = 0;

  /**
   * \brief This function is called by higher layer protocol when sending packets
   */
  virtual void Send6 (Ptr<Packet> packet, Ipv6Address source,
                     Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route) = 0;

  // From IpL4Protocol
  virtual void SetDownTarget (IpL4Protocol::DownTargetCallback cb);
  virtual void SetDownTarget6 (IpL4Protocol::DownTargetCallback6 cb);
  // From IpL4Protocol
  virtual IpL4Protocol::DownTargetCallback GetDownTarget (void) const;
  virtual IpL4Protocol::DownTargetCallback6 GetDownTarget6 (void) const;

protected:
  /**
   * \brief forward a packet to upper layers(without any change)
   * \param p packet to forward up
   * \param header IPv4 Header information
   * \param incomingInterface the Ipv4Interface on which the packet arrived
   * \param protocolNumber the protocol number of upper layer
   * \returns Rx status code
   */
  enum IpL4Protocol::RxStatus ForwardUp (Ptr<Packet> p,
                                         Ipv4Header const &header,
                                         Ptr<Ipv4Interface> incomingInterface,
                                         uint8_t protocolNumber);

  /**
   * \brief forward a packet to upper layers(without any change)
   * \param p packet to forward up
   * \param header IPv6 Header information
   * \param incomingInterface the Ipv6Interface on which the packet arrived
   * \param protocolNumber the protocol number of upper layer
   * \returns Rx status code
   */
  enum IpL4Protocol::RxStatus ForwardUp (Ptr<Packet> p,
                                         Ipv6Header const &header,
                                         Ptr<Ipv6Interface> incomingInterface,
                                         uint8_t protocolNumber);

  /**
   * \brief This function is called by subclass protocol when sending packets
   */
  void ForwardDown (Ptr<Packet> p, Ipv4Address source,
                    Ipv4Address destination, Ptr<Ipv4Route> route);
  /**
   * \brief This function is called by subclass protocol when sending packets
   */
  void ForwardDown (Ptr<Packet> p, Ipv6Address source,
                    Ipv6Address destination, Ptr<Ipv6Route> route);

  /**
   * This function will notify other components connected to the node that a new stack member is now connected
   * This will be used to notify Layer 3 protocol of layer 4 protocol stack to connect them together.
   */
  virtual void NotifyNewAggregate (void);
private:
  Ptr<Node> m_node;   //!< the node this stack is associated with
  IpL4Protocol::DownTargetCallback m_downTarget;   //!< Callback to send packets over IPv4
  IpL4Protocol::DownTargetCallback6 m_downTarget6; //!< Callback to send packets over IPv6
};

}

#endif // IP_L3_5_PROTOCOL_H
