#ifndef C3_ECN_RECORDER_H
#define C3_ECN_RECORDER_H

#include <cstdint>
#include <map>

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"

#include "c3-type.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 ecn recorder
 * ecn recorder to record the ratio of marked ip packets in receive end.
 */
class C3EcnRecorder : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3EcnRecorder ();

  virtual ~C3EcnRecorder ();

  /**
   * @brief Reset record
   */
  void Reset (void);

  /**
   * @brief GetRatio
   * @return marked/total ratio
   */
  double GetMarkedRatio (void) const;

  /**
   * @brief GetMarkedCount
   * @return marked count
   */
  uint32_t GetMarkedBytes (void) const;

  /**
   * @brief NotifyReceived
   * @param header
   * called by receiver when a ip packet is received
   */
  void NotifyReceived (Ipv4Header const &header);

  /**
   * @brief GetEcnRecorder
   * @param tenantId the tenant id
   * @param type objective type
   * @param src src addr
   * @param dst dst addr
   * @return ecn recorder if exists; else nullptr
   */
  static Ptr<C3EcnRecorder> GetEcnRecorder (uint32_t tenantId, C3Type type,
                                            const Ipv4Address &src, const Ipv4Address &dst);

  /**
   * @brief CreateEcnRecorder
   * @param tenantId the tenant id
   * @param type objective type
   * @param src src addr
   * @param dst dst addr
   * @return newly created ecn recorder
   */
  static Ptr<C3EcnRecorder> CreateEcnRecorder (uint32_t tenantId, C3Type type,
                                               const Ipv4Address &src, const Ipv4Address &dst);

private:

  uint32_t m_markedBytes;
  uint32_t m_totalBytes;

  class EcnRecorderListKey_t {
  public:

    EcnRecorderListKey_t (uint32_t tenantId, C3Type type,
                          const Ipv4Address &src, const Ipv4Address &dst);

    bool operator < (const EcnRecorderListKey_t &other) const;
    bool operator == (const EcnRecorderListKey_t &other) const;

  private:

    uint32_t m_tenantId;
    C3Type m_type;
    Ipv4Address m_src;
    Ipv4Address m_dst;
  };

  typedef std::map<EcnRecorderListKey_t, Ptr<C3EcnRecorder> > EcnRecorderList_t;

  static EcnRecorderList_t m_ecnRecorderList;

};

} //namespace dcn
} //namespace ns3

#endif // C3_ECN_RECORDER_H
