#include "d2tcp-socket-factory.h"

#include "d2tcp-socket.h"

#include "ns3/assert.h"
#include "ns3/socket.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (D2tcpSocketFactory);

TypeId
D2tcpSocketFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::D2tcpSocketFactory")
      .SetParent<DctcpSocketFactoryBase> ()
      .SetGroupName ("Internet")
      .AddConstructor<D2tcpSocketFactory> ();
  return tid;
}

Ptr<Socket>
D2tcpSocketFactory::CreateSocket (void)
{
  TypeIdValue congestionTypeId;
  GetTcp ()->GetAttribute ("SocketType", congestionTypeId);
  return GetTcp ()->CreateSocket (congestionTypeId.Get (), D2tcpSocket::GetTypeId ());
}

} // namespace ns3
