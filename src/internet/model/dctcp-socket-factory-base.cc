#include "dctcp-socket-factory-base.h"

#include "ns3/assert.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DctcpSocketFactoryBase);

TypeId
DctcpSocketFactoryBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DctcpSocketFactoryBase")
      .SetParent<SocketFactory> ()
      .SetGroupName ("Internet");
  return tid;
}

DctcpSocketFactoryBase::DctcpSocketFactoryBase ()
  : m_tcp (0)
{
}

DctcpSocketFactoryBase::~DctcpSocketFactoryBase ()
{
  NS_ASSERT (m_tcp == 0);
}

void
DctcpSocketFactoryBase::SetTcp (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

Ptr<TcpL4Protocol>
DctcpSocketFactoryBase::GetTcp (void)
{
  return m_tcp;
}

void
DctcpSocketFactoryBase::DoDispose (void)
{
  m_tcp = 0;
  SocketFactory::DoDispose ();
}

} // namespace ns3
