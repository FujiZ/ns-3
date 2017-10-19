#include "c3-ecn-recorder.h"

#include "ns3/log.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3EcnRecorder");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3EcnRecorder);

C3EcnRecorder::EcnRecorderList_t C3EcnRecorder::m_ecnRecorderList;

TypeId
C3EcnRecorder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3EcnRecorder")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3EcnRecorder> ();
  ;
  return tid;
}

C3EcnRecorder::C3EcnRecorder ()
  : m_markedBytes (0),
    m_totalBytes (0)
{
  NS_LOG_FUNCTION (this);
}

C3EcnRecorder::~C3EcnRecorder ()
{
  NS_LOG_FUNCTION (this);
}

void
C3EcnRecorder::Reset (void)
{
  NS_LOG_FUNCTION (this);
  m_markedBytes = m_totalBytes = 0;
}

double
C3EcnRecorder::GetMarkedRatio (void) const
{
  return m_markedBytes ? static_cast<double> (m_markedBytes) / m_totalBytes : 0.0;
}

uint32_t
C3EcnRecorder::GetMarkedBytes (void) const
{
  return m_markedBytes;
}

void
C3EcnRecorder::NotifyReceived (const Ipv4Header &header)
{
  NS_LOG_FUNCTION (this << header);
  m_totalBytes += header.GetPayloadSize ();
  if (header.GetEcn () == Ipv4Header::ECN_CE)
    {
      m_markedBytes += header.GetPayloadSize ();
    }
}

Ptr<C3EcnRecorder>
C3EcnRecorder::GetEcnRecorder (uint32_t tenantId, C3Type type,
                               const Ipv4Address &src, const Ipv4Address &dst)
{
  EcnRecorderListKey_t key (tenantId, type, src, dst);
  auto it = m_ecnRecorderList.find (key);
  if (it != m_ecnRecorderList.end ())
    {
      return it->second;
    }
  else
    {
      return 0;
    }
}

Ptr<C3EcnRecorder>
C3EcnRecorder::CreateEcnRecorder (uint32_t tenantId, C3Type type,
                                  const Ipv4Address &src, const Ipv4Address &dst)
{
  EcnRecorderListKey_t key (tenantId, type, src, dst);
  auto it = m_ecnRecorderList.find (key);
  if (it != m_ecnRecorderList.end ())
    {
      return it->second;
    }
  else
    {
      Ptr<C3EcnRecorder> ecnRecorder = CreateObject<C3EcnRecorder> ();
      m_ecnRecorderList[key] = ecnRecorder;
      return ecnRecorder;
    }
}

C3EcnRecorder::EcnRecorderListKey_t::EcnRecorderListKey_t (uint32_t tenantId, C3Type type,
                                                           const Ipv4Address &src, const Ipv4Address &dst)
  : m_tenantId (tenantId),
    m_type (type),
    m_src (src),
    m_dst (dst)
{
}

bool
C3EcnRecorder::EcnRecorderListKey_t::operator < (const C3EcnRecorder::EcnRecorderListKey_t &other) const
{
  return (m_tenantId < other.m_tenantId)
      || ((m_tenantId == other.m_tenantId)
          && (m_type < other.m_type))
      || ((m_tenantId == other.m_tenantId)
          && (m_type == other.m_type)
          && (m_src < other.m_src))
      || ((m_tenantId == other.m_tenantId)
          && (m_type == other.m_type)
          && (m_src == other.m_src)
          && (m_dst < other.m_dst));
}

bool
C3EcnRecorder::EcnRecorderListKey_t::operator == (const C3EcnRecorder::EcnRecorderListKey_t &other) const
{
  return (m_tenantId == other.m_tenantId)
      && (m_type == other.m_type)
      && (m_src == other.m_src)
      && (m_dst == other.m_dst);
}

} //namespace dcn
} //namespace ns3
