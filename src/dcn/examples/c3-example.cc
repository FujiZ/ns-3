#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/dcn-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("C3Example");

const uint32_t flowSize = 100000;
const Time deadline = Seconds (5.0);
const int port = 9;

void
SendTracer (Ptr<const Packet> packet)
{
  static FlowIdTag flowIdTag;
  dcn::C3Tag c3Tag;
  c3Tag.SetFlowSize (flowSize);
  c3Tag.SetDeadline (deadline);
  packet->AddPacketTag (flowIdTag);
  packet->AddPacketTag (c3Tag);
  packet->AddByteTag (c3Tag);
}

void
ReceiveTracer (Ptr<const Packet> packet,const Address &from)
{
  static int totalReceive = 0;
  dcn::C3Tag c3Tag;
  NS_ASSERT(packet->FindFirstMatchingByteTag (c3Tag));
  if (c3Tag.GetDeadline () <= Simulator::Now ())
    {
      totalReceive += packet->GetSize ();
      NS_LOG_INFO ("At " << Simulator::Now () << " receive " << totalReceive <<"/" << c3Tag.GetFlowSize ());
    }
  else
    {
      NS_LOG_INFO ("At " << Simulator::Now () << " deadline expired " << totalReceive <<"/" << c3Tag.GetFlowSize ());
    }
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("C3Example", LOG_LEVEL_INFO);
  LogComponentEnable ("C3L3_5Protocol", LOG_LEVEL_INFO);
  LogComponentEnable ("C3Division", LOG_LEVEL_INFO);
  LogComponentEnable ("C3DsTunnel", LOG_LEVEL_INFO);
  LogComponentEnable ("C3DsFlow", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::C3L3_5Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::UdpL4Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
  l3_5Helper.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Address receiverAddress = InetSocketAddress(interfaces.GetAddress (1), port);

  BulkSendHelper sender ("ns3::TcpSocketFactory", receiverAddress);
  sender.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  ApplicationContainer senderApps = sender.Install (nodes.Get (0));
  senderApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeCallback (&SendTracer));
  senderApps.Start (Seconds (2.0));
  senderApps.Stop (Seconds (50.0));

  PacketSinkHelper receiver ("ns3::TcpSocketFactory", receiverAddress);
  ApplicationContainer receiverApps = receiver.Install (nodes.Get (1));
  receiverApps.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&ReceiveTracer));
  receiverApps.Start (Seconds (1.0));
  receiverApps.Stop (Seconds (50.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
