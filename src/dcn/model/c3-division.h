#ifndef C3_DIVISION_H
#define C3_DIVISION_H

#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include "ns3/callback.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-route.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/timer.h"

#include "c3-tunnel.h"
#include "c3-type.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 division base class
 */
class C3Division : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Division (C3Type type);

  virtual ~C3Division ();

  /**
   * @brief GetDivision
   * @param tenantId tenant id (set in c3tag)
   * @param type objective type (set in c3tag)
   * @return division if exist; else 0
   */
  static Ptr<C3Division> GetDivision (uint32_t tenantId, C3Type type);

  /**
   * @brief CreateDivision according to its type
   * @param type objective type
   * @return newly created division
   */
  static Ptr<C3Division> CreateDivision (uint32_t tenantId, C3Type type);

  /**
   * @brief GetTunnel
   * @param src tunnel src addr
   * @param dst tunnel dst addr
   * @return required tunnel
   */
  virtual Ptr<C3Tunnel> GetTunnel (const Ipv4Address &src, const Ipv4Address &dst) = 0;


  /**
   * @brief AddDivisionType
   * @param type objective type
   * @param tid TypeId of division
   * the division MUST have a default constructor
   */
  static void AddDivisionType (C3Type type, std::string tid);

  /**
   * @brief Set start time
   * @param start time to start global division update
   */
  static void Start (Time start);

  /**
   * @brief Set stop time
   * @param stop time to stop global division update
   */
  static void Stop (Time stop);

  /**
   * @brief Set timer Interval
   * @param interval
   */
  static void SetInterval (Time interval);

  /**
   * @brief Global Update
   * update all divisions in the network
   */
  static void UpdateAll (void);

  /**
   * @brief Update division
   * update tunnels in divisions
   * called by global timer (?)
   */
  void Update (void);

protected:

  virtual void DoDispose (void);

  /**
   * @brief GetTenantId
   * @return tenant id of current division
   */
  uint32_t GetTenantId (void);

  typedef std::pair<Ipv4Address, Ipv4Address> TunnelListKey_t;
  typedef std::map<TunnelListKey_t, Ptr<C3Tunnel> > TunnelList_t;

  TunnelList_t m_tunnelList;    //!< tunnel list

private:

  uint32_t m_tenantId;  //!< tenant id of divison
  C3Type m_type;    //!< objective type
  double m_weight;  //!< division weight

  typedef std::pair<uint32_t, C3Type> DivisionListKey_t;
  typedef std::map<DivisionListKey_t, Ptr<C3Division> > DivisionList_t;
  typedef std::map<C3Type, std::string> DivisionTypeList_t;

  static DivisionList_t m_divisionList;
  static DivisionTypeList_t m_divisionTypeList;
  static Time m_startTime;  //!< start time of division
  static Time m_stopTime;   //!< stop time of division
  static Time m_interval;   //!< interval to call GlobalUpdate ()
  static Timer m_timer;     //!< timer to call GlobalUpdate ()
};

} //namespace dcn
} //namespace ns3

#endif // C3_DIVISION_H
