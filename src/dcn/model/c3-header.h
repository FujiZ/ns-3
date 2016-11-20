#ifndef C3_HEADER_H
#define C3_HEADER_H

#include <stdint.h>

#include "ns3/header.h"

namespace ns3 {
/**
 * \ingroup dcn
 * \brief Packet header for c3 packets
   \verbatim
   |      0        |      1        |      2        |      3        |
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Next Header |F|     Reservd    |       Payload Length       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   \endverbatim
 */
class C3Header : public Header
{
public:
  /**
   * \brief Constructor
   *
   * Creates a null header
   */
  C3Header();
  ~C3Header();

  /**
   * \brief Set the "Next header" field.
   * \param protocol the next header number
   */
  void SetNextHeader (uint8_t protocol);

  /**
   * \brief Get the next header.
   * \return the next header number
   */
  uint8_t GetNextHeader () const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  // From Header
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

private:
  uint8_t m_nextHeader;	//!< The "next header" field.
};

}

#endif // C3_HEADER_H
