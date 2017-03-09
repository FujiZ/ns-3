#ifndef C3_DIVISION_H
#define C3_DIVISION_H

#include <map>
#include <string>
#include <utility>

#include "ns3/callback.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-route.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include "c3-tunnel.h"
#include "c3-type.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 division implementation
 *
 */
class C3Division : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Division ();

  virtual ~C3Division ();

  /**
   * @brief GetDivision
   * @param tenantId tenant id (set in c3tag)
   * @param type objective type (set in c3tag)
   * @return division
   * Get division; new one if not exist
   */
  static Ptr<C3Division> GetDivision (uint32_t tenantId, C3Type type);

  /**
   * @brief AddDivisionType
   * @param type objective type
   * @param tid TypeId of division
   * the division MUST have a default constructor
   */
  static void AddDivisionType (C3Type type, std::string tid);

  /**
   * @brief Global Update
   * update all divisions in the network
  static void GlobalUpdate (void);
   */

  /**
   * @brief GetTunnel
   * @param src tunnel src addr
   * @param dst tunnel dst addr
   * @return required tunnel
   */
  virtual Ptr<C3Tunnel> GetTunnel (const Ipv4Address &src, const Ipv4Address &dst) = 0;

  /**
   * @brief Update division
   * update tunnels in divisions
   * called by global timer (?)
   */
  void Update (void);

protected:

  virtual void DoDispose (void);

  typedef std::pair<Ipv4Address, Ipv4Address> TunnelListKey_t;
  typedef std::map<TunnelListKey_t, Ptr<C3Tunnel> > TunnelList_t;

  double m_weight;  //!< division weight
  TunnelList_t m_tunnelList;    //!< tunnel list

private:

  /**
   * @brief CreateDivision according to its type
   * @param type objective type
   * @return newly created division
   */
  static Ptr<C3Division> CreateDivision (C3Type type);

  typedef std::pair<uint32_t, C3Type> DivisionListKey_t;
  typedef std::map<DivisionListKey_t, Ptr<C3Division> > DivisionList_t;
  typedef std::map<C3Type, std::string> DivisionTypeList_t;

  static DivisionList_t m_divisionList;
  static DivisionTypeList_t m_divisionTypeList;

  /*
  /// should be placed elsewhere(C3EcnHandler?)
  //ecn stats
  uint32_t m_totalAck;
  uint32_t m_markedAck;
  double m_alpha;
  double m_g;
  ///\todo incoming congestion status
  ///uint32_t m_incomingPackets;
  ///uint32_t m_incomingECNMarkedPackets;
  ///uint32_t m_incomingAlpha;
  ///uint32_t m_incomingG;

  ///\todo tunnel map|set
  //Ptr<C3DsTunnel> m_tunnel;     //!< \todo just one tunnel for test

  //std::map<C3Tag::Type, Ptr<C3Tunnel> > tunnelMap;
  //c3tag, c3dstag, c3cstag etc.
  */
};

} //namespace dcn
} //namespace ns3

#endif // C3_DIVISION_H
