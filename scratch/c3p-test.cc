#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dcn-module.h"

#include <string>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("C3pTest");

// attributes
std::string linkDataRate;
std::string linkDelay;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;
double client_interval_time;

// nodes
NodeContainer srcs;
NodeContainer routers;
NodeContainer dsts;

// dst interfaces
Ipv4InterfaceContainer dst_interfaces;

void
SetupName (const NodeContainer& nodes, const std::string& prefix)
{
  int i = 0;
  for(auto it = nodes.Begin (); it != nodes.End (); ++it)
    {
      std::stringstream ss;
      ss << prefix << i++;
      Names::Add (ss.str (), *it);
    }
}

void
SetupName (void)
{
  SetupName (srcs, "src");
  SetupName (routers, "r");
  SetupName (dsts, "dst");
}

void
SetupTopo (uint32_t srcNo, uint32_t dstNo, const DataRate& bandwidth, const Time& delay)
{
  NS_LOG_INFO ("Create nodes");
  srcs.Create (srcNo);
  routers.Create (2);
  dsts.Create (dstNo);

  NS_LOG_INFO ("Setup node name");
  SetupName ();

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (srcs);
  internet.Install (routers);
  internet.Install (dsts);

  Ipv4AddressHelper ipv4AddrHelper;
  ipv4AddrHelper.SetBase ("10.1.1.0", "255.255.255.0");

  PointToPointHelper p2pHelper;
  p2pHelper.SetQueue ("ns3::DropTailQueue");
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (bandwidth);
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (delay));

  TrafficControlHelper pfifoHelper;
  uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (90));
  pfifoHelper.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (90));

  NS_LOG_INFO ("Setup src nodes");
  for (auto it = srcs.Begin (); it != srcs.End (); ++it)
    {
      NetDeviceContainer devs = p2p.Install (NodeContainer (*it, switchs.Get (0)));
      pfifoHelper.Install (devs);
      ipv4AddrHelper.Assign (devs);
      ipv4AddrHelper.NewNetwork ();
    }
  NS_LOG_INFO ("Setup dst nodes");
  for (auto it = dsts.Begin (); it != dsts.End (); ++it)
    {
      NetDeviceContainer devs = p2p.Install (NodeContainer (switchs.Get (1), *it));
      pfifoHelper.Install (devs);
      dst_interfaces.Add (ipv4AddrHelper.Assign (devs).Get (1));
      ipv4AddrHelper.NewNetwork ();
    }
  NS_LOG_INFO ("Setup router nodes");
  {
    NetDeviceContainer devs = p2p.Install (routers);
    // only backbone link has RED queue disc
    TrafficControlHelper redHelper;
    redHelper.SetRootQueueDisc ("ns3::RedQueueDisc",
                                "LinkBandwidth", DataRateValue (bandwidth),
                                "LinkDelay", TimeValue (delay));
    redHelper.Install (devs);
    ipv4AddrHelper.Assign (devs);
  }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
SetupApp (bool enableLS, bool enableDS, bool enableCS)
{
  // SINK is in the dst side
  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (servers.Get (0));
  sinkApp.Start (Seconds (sink_start_time));
  sinkApp.Stop (Seconds (sink_stop_time));
  sinkApp.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));

  ///\todo set different socketbase to tcpl4protocol
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", InetSocketAddress (serverInterfaces.GetAddress (0), port));
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate (linkDataRate)));
  // clientHelper.SetAttribute ("PacketSize", UintegerValue (1000));

  ApplicationContainer clientApps = clientHelper.Install (clients);

  // set different start/stop time for each app
  double clientStartTime = client_start_time;
  double clientStopTime = client_stop_time;
  uint32_t i = 1;
  for (auto it = clientApps.Begin (); it != clientApps.End (); ++it)
    {
      Ptr<Application> app = *it;
      app->SetStartTime (Seconds (clientStartTime));
      app->SetStopTime (Seconds (clientStopTime));
      app->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, i++));
      clientStartTime += client_interval_time;
      clientStopTime -=  client_interval_time;
    }
}

void
SetConfig (void)
{

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  // RED params
  NS_LOG_INFO ("Set RED params");
  Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (500));
  Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  // Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (20));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (30));
  Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (250));
  Config::SetDefault ("ns3::RedQueueDisc::UseMarkP", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::MarkP", DoubleValue (2.0));

  // TCP params
  // Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  // Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

  // Config::SetDefault ("ns3::TcpL4Protocol::SocketBaseType", TypeIdValue(TypeId::LookupByName ("ns3::DctcpSocket")));
  // Config::SetDefault ("ns3::DctcpSocket::DctcpWeight", DoubleValue (1.0 / 16));
  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
}

int
main (int argc, char *argv[])
{

  global_start_time = 0.0;
  global_stop_time = 100.0;
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 3.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 1.0;
  client_interval_time = 20.0;

  linkDataRate = "100Mbps";
  linkDelay = "2ms";

  CommandLine cmd;

  cmd.Parse (argc, argv);

  SetupTopo (3, 3, );
  SetupApp (false, true, true);

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
