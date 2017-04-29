#ifndef C3_TAG_H
#define C3_TAG_H

#include <stdint.h>

#include "ns3/tag.h"
#include "ns3/nstime.h"

#include "c3-type.h"

namespace ns3 {
namespace dcn {

class C3Tag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Tag ();
  ~C3Tag ();

  // inherited from Tag
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;


  void SetType (C3Type type);
  C3Type GetType (void) const;

  void SetTenantId (uint32_t tenantId);
  uint32_t GetTenantId (void) const;

  void SetFlowSize (uint32_t flowSize);
  uint32_t GetFlowSize (void) const;

  void SetPacketSize (uint32_t packetSize);
  uint32_t GetPacketSize (void) const;

  void SetDeadline (Time deadline);
  Time GetDeadline (void) const;

  bool operator == (const C3Tag &other) const;
  bool operator != (const C3Tag &other) const;

private:
  C3Type m_type;        //!< objective type of current flow
  uint32_t m_tenantId;  //!< tenant id of current flow
  uint32_t m_flowSize;   //!< the size of current flow(in Byte)
  uint32_t m_packetSize; //!< the size of packet
  Time m_deadline;       //!< deadline of flow(used in DS flow)
};

} //namespace dcn
} //namespace ns3

#endif // C3_TAG_H
