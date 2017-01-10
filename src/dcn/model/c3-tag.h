#ifndef C3_TAG_H
#define C3_TAG_H

#include <stdint.h>

#include "ns3/tag.h"
#include "ns3/nstime.h"

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

  // inherited from Tag
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;

  C3Tag ();

  /**
   * Construct a C3Tag with given args
   * \param flowSize Size of Flow
   * \param packetSize Size of Data Field
   */
  C3Tag (uint32_t flowSize, uint32_t packetSize);
  /**
   * Sets the flow size for the tag
   * \param flowSize size to assign to the tag
   */
  void SetFlowSize (uint32_t flowSize);
  /**
   * Gets the flow size for the tag
   * \returns current flow size for this tag
   */
  uint32_t GetFlowSize (void) const;
  /**
   * Sets the packet size for the tag
   * \param packetSize size to assign to the tag
   */
  void SetPacketSize (uint32_t packetSize);
  /**
   * Gets the packet size for the tag
   * \returns current packet size for this tag
   */
  uint32_t GetPacketSize (void) const;
  /**
   * Sets the deadline for the tag
   * \param deadline deadline to assign to the tag
   * used in DS flow
   */
  void SetDeadline (Time deadline);
  /**
   * Gets the deadline for the tag
   * \returns current deadline for this tag
   * used in DS flow
   */
  Time GetDeadline (void) const;

private:
  uint32_t m_flowSize;   //!< the size of current flow(in Byte)
  uint32_t m_packetSize; //!< the size of packet
  Time m_deadline;       //!< deadline of flow(used in DS flow)
};

} //namespace dcn
} //namespace ns3

#endif // C3_TAG_H
