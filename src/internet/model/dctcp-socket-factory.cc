#include "dctcp-socket-factory.h"

#include "dctcp-socket.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DctcpSocketFactory);

TypeId
DctcpSocketFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DctcpSocketFactory")
      .SetParent<DctcpSocketFactoryBase> ()
      .SetGroupName ("Internet")
      .AddConstructor<DctcpSocketFactory> ();
  return tid;
}

Ptr<Socket>
DctcpSocketFactory::CreateSocket (void)
{
  TypeIdValue congestionTypeId;
  GetTcp ()->GetAttribute ("SocketType", congestionTypeId);
  Ptr<Socket> socket = GetTcp ()->CreateSocket (congestionTypeId.Get (), DctcpSocket::GetTypeId ());
  socket->SetAttribute ("UseEcn", BooleanValue (true));
  return socket;
}

} // namespace ns3
