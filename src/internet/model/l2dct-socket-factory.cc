#include "l2dct-socket-factory.h"

#include "l2dct-socket.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (L2dctSocketFactory);

TypeId
L2dctSocketFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::L2dctSocketFactory")
      .SetParent<DctcpSocketFactoryBase> ()
      .SetGroupName ("Internet")
      .AddConstructor<L2dctSocketFactory> ();
  return tid;
}

Ptr<Socket>
L2dctSocketFactory::CreateSocket (void)
{
  TypeIdValue congestionTypeId;
  GetTcp ()->GetAttribute ("SocketType", congestionTypeId);
  Ptr<Socket> socket = GetTcp ()->CreateSocket (congestionTypeId.Get (), L2dctSocket::GetTypeId ());
  socket->SetAttribute ("UseEcn", BooleanValue (true));
  return socket;
}

} // namespace ns3
