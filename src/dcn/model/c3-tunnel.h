#ifndef C3_TUNNEL_H
#define C3_TUNNEL_H

#include <cstdint>
#include <map>

#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include "c3-flow.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the base class for various type of tunnel (eg: LS, DS)
 */
class C3Tunnel : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Tunnel (const Ipv4Address &src, const Ipv4Address &dst);

  virtual ~C3Tunnel ();

  /**
   * \brief set the route of connection
   * \param route new route info
   */
  void SetRoute (Ptr<Ipv4Route> route);

  /**
   * \brief callback to forward packets to dest
   */
  typedef Callback<void, Ptr<Packet>, Ipv4Address, Ipv4Address, uint8_t, Ptr<Ipv4Route> > ForwardTargetCallback;

  /**
   * \brief set forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);

  /**
   * @brief Send a packet
   * @param packet packet tobe sent
   * @param protocol protocol number of transport layer
   */
  virtual void Send (Ptr<Packet> packet, uint8_t protocol) = 0;

  /**
   * @brief Update tunnel info (weight)
   * called by upper division
   * @todo maybe the update of weight and rate should seperate
   */
  virtual void UpdateInfo (void) = 0;

  double GetWeightRequest (void) const;

  /**
   * @brief SetWeightResponse
   * @param weight
   * called by division, set tunnel weight response
   */
  void SetWeightResponse (double weight);

  /**
   * @brief Update tunnel rate;
   * update tunnel rate according to ecn info
   */
  virtual void UpdateRate (void) = 0;

  /**
   * @brief in-tunnel schedule
   * alloc tunnel rate to flows inside the tunnel
   * scale flow rate inside the tunnel
   */
  virtual void Schedule (void) = 0;

protected:

  virtual void DoDispose (void);

  /**
   * \brief forward a packet to dest
   * \param packet packet to be forward
   * \param protocol flow protocol number
   * usually used as callback
   */
  void Forward (Ptr<Packet> packet, uint8_t protocol);

  typedef std::map<uint32_t, Ptr<C3Flow> > FlowList_t;

  double m_weight;  //!< real tunnel weight
  double m_weightRequest;   //!< tunnel weight request
  FlowList_t m_flowList;    //!< flow list

private:
  Ipv4Address m_src;   //!< src address of tunnel
  Ipv4Address m_dst;  //!< dst address of tunnel
  Ptr<Ipv4Route> m_route; //!< route of connection
  ForwardTargetCallback m_forwardTarget;  //!< forward target
  /* rate
   * weight
   * flow list
   */
};

} //namespace dcn
} //namespace ns3

#endif // C3_TUNNEL_H
