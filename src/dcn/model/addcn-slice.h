#ifndef ADDCN_SLICE_H
#define ADDCN_SLICE_H

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
#include "ns3/ipv4-flow-classifier.h"

#include "addcn_flow.h"
#include "c3-type.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 slice base class
 */
class ADDCNSlice : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ADDCNSlice (C3Type type);

  virtual ~ADDCNSlice ();

  /**
   * @brief GetSlice
   * @param tenantId tenant id (set in c3tag)
   * @param type objective type (set in c3tag)
   * @return slice if exist; else 0
   */
  static Ptr<ADDCNSlice> GetSlice (uint32_t tenantId, C3Type type);

  /**
   * @brief CreateSlice according to its type
   * @param type objective type
   * @return newly created slice
   */
  static Ptr<ADDCNSlice> CreateSlice (uint32_t tenantId, C3Type type);

  /**
   * @brief GetFlow
   * @param tup <srcIP, srcPort, dstIP, dstPort, protocol> tuple
   * @return required flow
   */
  static Ptr<ADDCNFlow> GetFlow (const Ipv4FlowClassifier::FiveTuple &tup)

   // static Ptr<ADDCNFlow> GetFlow(const Ipv4Address &src, const Ipv4Address &dst, uint32_t srcPort, uint32_t dstPort, uint8_t protocol);


  /**
   * @brief AddSliceType
   * @param type objective type
   * @param tid TypeId of slice
   * the slice MUST have a default constructor
   */
  static void AddSliceType (C3Type type, std::string tid);

  /**
   * @brief Set start time
   * @param start time to start global slice update
   */
  static void Start (Time start);

  /**
   * @brief Set stop time
   * @param stop time to stop global slice update
   */
  static void Stop (Time stop);

  /**
   * @brief Set timer Interval
   * @param interval
   */
  static void SetInterval (Time interval);

  /**
   * @brief Global Update
   * update all slices in the network
   */
  static void UpdateAll (void);

  /**
   * @brief Update slice
   * update tunnels in slices
   * called by global timer (?)
   */
  void Update (void);

protected:

  virtual void DoDispose (void);

  /**
   * @brief GetTenantId
   * @return tenant id of current slice
   */
  uint32_t GetTenantId (void);

  typedef std::map<Ipv4FlowClassifier::FiveTuple, Ptr<ADDCNFlow>> FlowList_t

  FlowList_t m_flowList;

private:

  uint32_t m_tenantId;  //!< tenant id of slice
  C3Type m_type;    //!< objective type
  double m_weight;  //!< slice weight

  typedef std::pair<uint32_t, C3Type> SliceListKey_t;
  typedef std::map<SliceListKey_t, Ptr<ADDCNSlice> > SliceList_t;
  typedef std::map<C3Type, std::string> SliceTypeList_t;

  static SliceList_t m_sliceList;
  static SliceTypeList_t m_sliceTypeList;
  static Time m_startTime;  //!< start time of slice
  static Time m_stopTime;   //!< stop time of slice
  static Time m_interval;   //!< interval to call GlobalUpdate ()
  static Timer m_timer;     //!< timer to call GlobalUpdate ()
};

} //namespace dcn
} //namespace ns3

#endif // ADDCN_SLICE_H
