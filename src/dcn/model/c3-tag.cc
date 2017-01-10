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
  return sizeof (m_flowSize)
      + sizeof (m_packetSize)
      + sizeof (double);
}
void
C3Tag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU32 (m_flowSize);
  buf.WriteU32 (m_packetSize);
  buf.WriteDouble (m_deadline.ToDouble (Time::S));  ///time resolution in second
}
void
C3Tag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_flowSize = buf.ReadU32 ();
  m_packetSize = buf.ReadU32 ();
  m_deadline = Time::FromDouble (buf.ReadDouble (), Time::S);
}
void
C3Tag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "C3 INFO [FlowSize: " << m_flowSize;
  os << ", PacketSize:" << m_packetSize;
  os << ", Deadline:" << m_deadline;
  os << "] ";
}
C3Tag::C3Tag ()
  : Tag ()
{
  NS_LOG_FUNCTION (this);
}

C3Tag::C3Tag (uint32_t flowSize, uint32_t packetSize)
  : Tag (),
    m_flowSize (flowSize),
    m_packetSize (packetSize)
{
  NS_LOG_FUNCTION (this << flowSize << packetSize);
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
  NS_LOG_FUNCTION (this);
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
  NS_LOG_FUNCTION (this);
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
  NS_LOG_FUNCTION (this);
  return m_deadline;
}

} //namespace dcn
} //namespace ns3
