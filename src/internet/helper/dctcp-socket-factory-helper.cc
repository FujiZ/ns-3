#include "dctcp-socket-factory-helper.h"

#include "ns3/dctcp-socket-factory-base.h"
#include "ns3/object-factory.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

DctcpSocketFactoryHelper::~DctcpSocketFactoryHelper ()
{
  m_socketFactorys.clear ();
}

void
DctcpSocketFactoryHelper::AddSocketFactory (const std::string &tid)
{
  m_socketFactorys.push_back (TypeId::LookupByName (tid));
}

void
DctcpSocketFactoryHelper::Install (NodeContainer nodes)
{
  for (NodeContainer::Iterator it = nodes.Begin ();
       it != nodes.End (); ++it)
    {
      Install (*it);
    }
}

void
DctcpSocketFactoryHelper::Install (Ptr<Node> node)
{
  Ptr<TcpL4Protocol> tcp = node->GetObject<TcpL4Protocol> ();
  NS_ASSERT (tcp);
  ObjectFactory factory;
  for (auto &tid : m_socketFactorys)
    {
      factory.SetTypeId (tid);
      Ptr<DctcpSocketFactoryBase> socketFactory = factory.Create<DctcpSocketFactoryBase> ();
      NS_ASSERT (socketFactory);
      socketFactory->SetTcp (tcp);
      node->AggregateObject (socketFactory);
    }
}

} // namespace ns3
