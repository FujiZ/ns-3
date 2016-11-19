#include "c3-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (UdpHeader);

C3Header::C3Header ()
  : m_nextHeader(0)
{
}

C3Header::~C3Header ()
{
}

void
C3Header::SetNextHeader (uint8_t protocol)
{
  m_nextHeader = protocol;
}

uint8_t
C3Header::GetNextHeader ()
{
  return m_nextHeader;
}

TypeId
C3Header::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::C3Header")
    .SetParent<Header> ()
    .SetGroupName ("DCN")
    .AddConstructor<C3Header> ()
  ;
  return tid;
}

uint32_t
C3Header::GetSerializedSize (void) const
{
  return 1;
}

void
C3Header::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_nextHeader);
}

uint32_t
C3Header::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_nextHeader = i.ReadU8 ();

  return GetSerializedSize ();
}

void
C3Header::Print (std::ostream &os) const
{
  os << "nextHeader: " << (uint32_t)GetNextHeader ();
}

} // namespace ns3
