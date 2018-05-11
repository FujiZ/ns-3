// DCN - Layer 3.5 Protocol base class
// Fuji Z, Winter 2016
#ifndef IP_L3_5_PROTOCOL_H
#define IP_L3_5_PROTOCOL_H

#include <stdint.h>
#include <map>
#include <utility>

#include "ns3/ip-l4-protocol.h"
#include "ns3/type-id.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-interface.h"

namespace ns3 {
namespace dcn {

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

  IpL3_5Protocol ();
  /** Dummy destructor, see DoDispose. */
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

  // From IpL4Protocol
  virtual int GetProtocolNumber (void) const;
  /**
   * @brief the setter of protocol number
   * @param protocolNumber
   * 为了能够在不需要额外header的前提下在传输层和网络层间插入代理，
   * 需要在调用IpL3Protocol::Insert之前设置需要代理的protocol number。
   */
  void SetProtocolNumber (uint8_t protocolNumber);

  // inspired by ipv4-l3-protocol
  void Insert (Ptr<IpL4Protocol> protocol);
  void Insert (Ptr<IpL4Protocol> protocol, int32_t interfaceIndex);

  void Remove (Ptr<IpL4Protocol> protocol);
  void Remove (Ptr<IpL4Protocol> protocol, int32_t interfaceIndex);

  Ptr<IpL4Protocol> GetProtocol (int protocolNumber) const;
  Ptr<IpL4Protocol> GetProtocol (int protocolNumber, int32_t interfaceIndex) const;


protected:

  virtual void DoDispose (void);
  /*
   * This function will notify other components connected to the node that a new stack member is now connected
   * This will be used to notify Layer 3 protocol of layer 4 protocol stack to connect them together.
   */
  virtual void NotifyNewAggregate (void);

  /**
   * \brief forward a packet to upper layers (without any change)
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
  enum IpL4Protocol::RxStatus ForwardUp6 (Ptr<Packet> p,
                                         Ipv6Header const &header,
                                         Ptr<Ipv6Interface> incomingInterface,
                                         uint8_t protocolNumber);

  /**
   * @brief This function is called by subclass protocol when sending packets
   * @param p packet tobe send
   * @param source src addr
   * @param destination dst addr
   * @param protocol l4 protocol number
   * @param route route of the packet
   */
  void ForwardDown (Ptr<Packet> p,
                    Ipv4Address src, Ipv4Address dst,
                    uint8_t protocol, Ptr<Ipv4Route> route);

  /**
   * @brief This function is called by subclass protocol when sending packets
   * @param p packet tobe send
   * @param source src addr
   * @param destination dst addr
   * @param protocol l4 protocol number
   * @param route route of the packet
   */
  void ForwardDown6 (Ptr<Packet> p,
                     Ipv6Address src, Ipv6Address dst,
                     uint8_t protocol, Ptr<Ipv6Route> route);

private:
  /**
   * \brief Container of the IPv4 L4 keys: protocol number, interface index
   */
  typedef std::pair<int, int32_t> L4ListKey_t;

  /**
   * \brief Container of the IPv4 L4 instances.
   */
  typedef std::map<L4ListKey_t, Ptr<IpL4Protocol> > L4List_t;

  uint8_t m_protocolNumber; //!< current protocol number.
  L4List_t m_protocols;  //!< List of transport protocol.
  Ptr<Node> m_node;   //!< the node this stack is associated with
  IpL4Protocol::DownTargetCallback m_downTarget;   //!< Callback to send packets over IPv4
  IpL4Protocol::DownTargetCallback6 m_downTarget6; //!< Callback to send packets over IPv6
};

} //namespace dcn
} //namespace ns3

#endif // IP_L3_5_PROTOCOL_H
