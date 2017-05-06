#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/dcn-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("C3pExample");

void
TxTrace (Ptr<const Packet> packet)
{
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << packet->GetSize () << " bytes");
}

void
RxTrace (Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes");
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("C3pExample", LOG_LEVEL_INFO);
  //LogComponentEnable ("DctcpSocket", LOG_LEVEL_INFO);
  //LogComponentEnable ("C3L3_5Protocol", LOG_LEVEL_INFO);

  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  //stack.SetTcp ("ns3::TcpL4Protocol", "SocketBaseType", TypeIdValue(TypeId::LookupByName ("ns3::dcn::DctcpSocket")));
  stack.Install (nodes);

  dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::C3L3_5Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::UdpL4Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
  l3_5Helper.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Address receiverAddress = InetSocketAddress(interfaces.GetAddress (1), 10);

  PacketSinkHelper receiver ("ns3::TcpSocketFactory", receiverAddress);
  ApplicationContainer receiverApps = receiver.Install (nodes.Get (1));
  receiverApps.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
  receiverApps.Start (Seconds (1.0));
  receiverApps.Stop (Seconds (10.0));

  BulkSendHelper sender ("ns3::TcpSocketFactory", receiverAddress);
  sender.SetAttribute ("MaxBytes", UintegerValue (3000));
  ApplicationContainer senderApps = sender.Install (nodes.Get (0));
  senderApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeCallback (&TxTrace));
  senderApps.Start (Seconds (2.0));
  senderApps.Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
