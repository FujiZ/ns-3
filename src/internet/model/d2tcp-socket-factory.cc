#include "d2tcp-socket-factory.h"

#include "d2tcp-socket.h"

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
  Ptr<Socket> socket = GetTcp ()->CreateSocket (congestionTypeId.Get (), D2tcpSocket::GetTypeId ());
  socket->SetAttribute ("UseEcn", BooleanValue (true));
  return socket;
}

} // namespace ns3
