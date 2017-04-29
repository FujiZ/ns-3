#include "c3-tag.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Tag");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Tag);

TypeId
C3Tag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Tag")
    .SetParent<Tag> ()
    .SetGroupName("DCN")
    .AddConstructor<C3Tag> ()
  ;
  return tid;
}

C3Tag::C3Tag ()
  : Tag (),
    m_type (C3Type::NONE),
    m_tenantId (0),
    m_flowSize (0),
    m_packetSize (0),
    m_deadline ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

C3Tag::~C3Tag ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId
C3Tag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
C3Tag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  ///\todo update size when add new member
  return sizeof (m_type)
      + sizeof (m_tenantId)
      + sizeof (m_flowSize)
      + sizeof (m_packetSize)
      + sizeof (double);
}

void
C3Tag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU8 (static_cast<uint8_t> (m_type));
  buf.WriteU32 (m_tenantId);
  buf.WriteU32 (m_flowSize);
  buf.WriteU32 (m_packetSize);
  buf.WriteDouble (m_deadline.GetSeconds ());  ///time resolution in second
}

void
C3Tag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_type = static_cast<C3Type> (buf.ReadU8 ());
  m_tenantId = buf.ReadU32 ();
  m_flowSize = buf.ReadU32 ();
  m_packetSize = buf.ReadU32 ();
  m_deadline = Time::FromDouble (buf.ReadDouble (), Time::S);
}

void
C3Tag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "C3 INFO [FlowSize: " << m_flowSize;
  os << ", PacketSize: " << m_packetSize;
  os << ", Deadline: " << m_deadline;
  os << "] ";
}

void
C3Tag::SetType (C3Type type)
{
  NS_LOG_FUNCTION (this << static_cast<uint8_t> (type));
  m_type = type;
}

C3Type
C3Tag::GetType (void) const
{
  return m_type;
}

void
C3Tag::SetTenantId (uint32_t tenantId)
{
  NS_LOG_FUNCTION (this << tenantId);
  m_tenantId = tenantId;
}

uint32_t
C3Tag::GetTenantId (void) const
{
  return m_tenantId;
}

void
C3Tag::SetFlowSize (uint32_t flowSize)
{
  NS_LOG_FUNCTION (this << flowSize);
  m_flowSize = flowSize;
}

uint32_t
C3Tag::GetFlowSize (void) const
{
  return m_flowSize;
}

void
C3Tag::SetPacketSize (uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << packetSize);
  m_packetSize = packetSize;
}

uint32_t
C3Tag::GetPacketSize (void) const
{
  return m_packetSize;
}

void
C3Tag::SetDeadline (Time deadline)
{
  NS_LOG_FUNCTION (this << deadline);
  m_deadline = deadline;
}

Time
C3Tag::GetDeadline (void) const
{
  return m_deadline;
}

bool
C3Tag::operator == (const C3Tag &other) const
{
  return m_type == other.m_type
      && m_tenantId == other.m_tenantId
      && m_flowSize == other.m_flowSize
      && m_packetSize == other.m_packetSize
      && m_deadline == other.m_deadline;
}

bool
C3Tag::operator != (const C3Tag &other) const
{
  return !operator == (other);
}

} //namespace dcn
} //namespace ns3
