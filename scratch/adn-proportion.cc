#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dcn-module.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ADN_PROPORTION");

uint32_t dumbell_host_num = 16;
NodeContainer srcs, dsts, routers;
Ipv4InterfaceContainer src_interfaces;
Ipv4InterfaceContainer dst_interfaces;

Time link_delay ("0.0375ms");
DataRate btnk_bw ("1Gbps");
DataRate non_btnk_bw ("10Gbps");
uint32_t queue_size = 250;
uint32_t threhold = 65;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;
double client_interval_time;

// c3 attributes
Time tunnel_interval ("1ms");
Time division_interval ("3ms");
DataRate tunnel_bw ("500Mbps"); //!< initial tunnel bw

// time attributes
double sim_time = 15;

// attributes
uint32_t packet_size = 1000;
uint16_t sink_port = 50000;
int64_t random_seed = 2;

uint32_t FlowIdBaseA = 100000;
uint32_t FlowIdBaseB = 200000;

std::map<uint32_t, uint64_t> totalRx;
std::map<uint32_t, uint64_t> lastRx;
// throughput result
std::map<uint32_t, std::vector<std::pair<double, double> > > throughputResult; //fId -> list<time, throughput>

void
SetupConfig (void)
{
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // RED params
  Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (packet_size));
  // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
  Config::SetDefault ("ns3::RedQueueDisc::UseMarkP", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::MarkP", DoubleValue (2.0));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (threhold));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (threhold));
  Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (queue_size));

  // TBF params
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::DataRate", DataRateValue (tunnel_bw));
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::Bucket", UintegerValue (83500));
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::QueueLimit", UintegerValue (queue_size));

  // TCP params
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packet_size));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));    //!< disable delayed ack
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

  // DCTCP params
  Config::SetDefault ("ns3::DctcpSocket::DctcpWeight", DoubleValue (1.0 / 16));

  // enable ECN
  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));

  // ADDCN params
  Config::SetDefault ("ns3::dcn::ADDCNFlow::DisableReorder", BooleanValue (false));

  // C3 params
  /*
  Config::SetDefault ("ns3::dcn::C3Division::Interval", TimeValue (division_interval));
  Config::SetDefault ("ns3::dcn::C3Division::RateThresh", DataRateValue (btnk_bw));

  Config::SetDefault ("ns3::dcn::C3Tunnel::Interval", TimeValue (tunnel_interval));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Gamma", DoubleValue (1.0 / 16));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Rate", DataRateValue (tunnel_bw));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MaxRate", DataRateValue (btnk_bw));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MinRate", DataRateValue (DataRate ("1Mbps")));
  Config::SetDefault ("ns3::dcn::C3Tunnel::RateThresh", DataRateValue (tunnel_bw));
  */
}

Ipv4InterfaceContainer
MakeLink (const std::string &type, Ptr<Node> src, Ptr<Node> dst,
          DataRate bw, Time delay, Ipv4AddressHelper &ipv4AddrHelper)
{
  PointToPointHelper p2pHelper;
  p2pHelper.SetQueue ("ns3::DropTailQueue");
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (delay));
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (bw));
  NetDeviceContainer devs = p2pHelper.Install (src, dst);
  auto interfaces = ipv4AddrHelper.Assign (devs);
  ipv4AddrHelper.NewNetwork ();

  TrafficControlHelper trafficHelper;
  if (type == "pfifo")
    {
      uint16_t handle = trafficHelper.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (1000));
      trafficHelper.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));
    }
  else if (type == "red")
    {
      ///\todo 对于10Gbps的链路 RED的设置是不是不同?
      /// 所有RED都是10Gbps的
      trafficHelper.SetRootQueueDisc ("ns3::RedQueueDisc",
                                      "LinkBandwidth", DataRateValue (bw),
                                      "LinkDelay", TimeValue (delay));
    }
  else
    {
      std::cerr << "Undefined QueueDisc: " << type << std::endl;
      std::exit (-1);
    }
    trafficHelper.Install (devs);
    return interfaces;
}

void
SetupDumbellTopo (uint32_t hostNum,
                  DataRate host2Switch, DataRate switch2Switch, Time linkDelay)
{
  InternetStackHelper internetHelper;
  Ipv4AddressHelper ipv4AddrHelper ("10.1.1.0", "255.255.255.0");

  srcs.Create (hostNum);
  routers.Create (2);
  dsts.Create (hostNum);

  internetHelper.Install (srcs);
  internetHelper.Install (routers);
  internetHelper.Install (dsts);

  for (NodeContainer::Iterator i = srcs.Begin (); i != srcs.End (); i++)
    {
      auto interfaces = MakeLink ("pfifo", *i, routers.Get (0), host2Switch, linkDelay, ipv4AddrHelper);
      src_interfaces.Add (interfaces.Get (0));
    }

  MakeLink ("red", routers.Get (0), routers.Get(1), switch2Switch, linkDelay, ipv4AddrHelper);

  for (NodeContainer::Iterator i = dsts.Begin (); i != dsts.End (); i++)
    {
      auto interfaces = MakeLink ("pfifo", routers.Get (1), *i, host2Switch, linkDelay, ipv4AddrHelper);
      dst_interfaces.Add (interfaces.Get (1));
    }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
RxTrace (Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (packet << from);
  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  uint32_t fid = flowIdTag.GetFlowId ();
  if (totalRx.find (fid) != totalRx.end ())
    {
      totalRx[fid] += packet->GetSize ();
    }
  else
    {
      totalRx[fid] = packet->GetSize ();
      lastRx[fid] = 0;
    }

  if (fid >= 200000)
    totalRx[1] += packet->GetSize (); // for tenantB
  else
    totalRx[0] += packet->GetSize (); // for tenantA
}

void
CalculateThroughput (void)
{
  for (auto it = totalRx.begin (); it != totalRx.end (); ++it)
    {
      double cur = (it->second - lastRx[it->first]) * (double) 8/1e8; /* Convert Application RX Packets to GBits. */
      throughputResult[it->first].push_back (std::pair<double, double> (Simulator::Now ().GetSeconds (), cur));
      lastRx[it->first] = it->second;
    }
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
InstallSink (NodeContainer nodes, uint16_t port, const Time &startTime, const Time &stopTime)
{
  PacketSinkHelper sinkHelper ("ns3::DctcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sinkHelper.Install (nodes);
  sinkApps.Start (startTime);
  sinkApps.Stop (stopTime);
  for (auto it = sinkApps.Begin (); it != sinkApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }
}

// LS App
void
InstallLsClient (Ptr<Node> node, const Address &sinkAddr, uint32_t tenantId,
                 uint32_t flowId, uint64_t flowSize, uint64_t packetSize,
                 const Time &startTime, const Time &stopTime)
{
  dcn::AddcnBulkSendHelper clientHelper ("ns3::DctcpSocketFactory", sinkAddr);
  //clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  auto it = clientApp.Begin();
  Ptr<dcn::AddcnBulkSendApplication> app = StaticCast<dcn::AddcnBulkSendApplication> (*it);

  app->SetTenantId (tenantId);
  app->SetFlowType (dcn::C3Type::LS);
  app->SetFlowId (flowId);
  app->SetFlowSize (flowSize);
  app->SetSegSize (packetSize);
}

int
main (int argc, char *argv[])
{
  int size_tenantA = 50;
  double weight_tenantA = 0.5;
  int size_tenantB = 50;
  double weight_tenantB = 0.5;
  std::string pathOut ("."); // Current directory

  global_start_time = 0.0;
  global_stop_time = 20.0;
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 1.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 1.0;
  client_interval_time = 5.0;

  CommandLine cmd;
  cmd.AddValue ("packetSize", "Size for every packet", packet_size);
  cmd.AddValue ("queueSize", "Queue length for RED queue", queue_size);
  cmd.AddValue ("threhold", "Threhold for RED queue", threhold);
  cmd.AddValue ("simTime", "simulation time", sim_time);
  cmd.AddValue ("pathOut", "Path to save results", pathOut);
  cmd.AddValue ("sizeTenantA", "Flow number of tenant A", size_tenantA);
  cmd.AddValue ("sizeTenantB", "Flow number of tenant B", size_tenantB);
  cmd.AddValue ("weightTenantA", "Global weight for tenant A", weight_tenantA);
  cmd.Parse (argc, argv);

  weight_tenantB = 1 - weight_tenantA;

  double scale = (weight_tenantA * size_tenantB) / (weight_tenantB * size_tenantA);

  totalRx[0] = 0;
  totalRx[1] = 0;
  lastRx[0] = 0;
  lastRx[1] = 0;

  if (scale < 0.1 || scale > 10.0)
  {
    printf("Illegal size & weight");
    return -1;
  }

  SetupConfig ();
  SetupDumbellTopo (dumbell_host_num, non_btnk_bw, btnk_bw, link_delay);

  // install dctcp socket factory to hosts
  DctcpSocketFactoryHelper dctcpHelper;
  dctcpHelper.AddSocketFactory ("ns3::DctcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::D2tcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::L2dctSocketFactory");
  dctcpHelper.Install (srcs);
  dctcpHelper.Install (dsts);

  Ptr<ns3::dcn::ADDCNSlice> sliceA = ns3::dcn::ADDCNSlice::GetSlice(0, dcn::C3Type::LS);
  Ptr<ns3::dcn::ADDCNSlice> sliceB = ns3::dcn::ADDCNSlice::GetSlice(1, dcn::C3Type::LS);
  
  sliceA->SetScale(scale);
  sliceB->SetScale(1.0);

  dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::ADDCNL3_5Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
  l3_5Helper.Install(srcs);
  l3_5Helper.Install(dsts);

/*
      ns3::dcn::ADDCNSlice::SetInterval(MilliSeconds (2));
      ns3::dcn::ADDCNSlice::Start(Seconds(globalStartTime));
      ns3::dcn::ADDCNSlice::Stop(Seconds(globalStopTime));
*/
  // install ip sink on all hosts
  InstallSink (dsts, sink_port, Seconds (sink_start_time), Seconds (sink_stop_time));

  auto hostStream = CreateObject<UniformRandomVariable> ();
  hostStream->SetStream (2);
  hostStream->SetAttribute ("Min", DoubleValue (0));
  hostStream->SetAttribute ("Max", DoubleValue (srcs.GetN () - 1));

  for (int i = 0; i < size_tenantA; i++)
    {
      uint32_t srcIndex = hostStream->GetInteger ();
      uint32_t dstIndex = hostStream->GetInteger ();
      uint32_t flowId = FlowIdBaseA++;
      InstallLsClient (srcs.Get (srcIndex), InetSocketAddress (dst_interfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, 0, packet_size, Seconds (client_start_time), Seconds (client_stop_time));
    }

  for (int i = 0; i < size_tenantB; i++)
    {
      uint32_t srcIndex = hostStream->GetInteger ();
      uint32_t dstIndex = hostStream->GetInteger ();
      uint32_t flowId = FlowIdBaseB++;
      InstallLsClient (srcs.Get (srcIndex), InetSocketAddress (dst_interfaces.GetAddress (dstIndex), sink_port), 1,
                           flowId, 0, packet_size, Seconds (client_start_time), Seconds (client_stop_time));
    }

  Simulator::Schedule (Seconds (sink_start_time), &CalculateThroughput);
  Simulator::Stop (Seconds(sink_stop_time));
  Simulator::Run ();

  for (auto& resultList : throughputResult)
    {
      std::stringstream ss;
      ss << pathOut << "/throughput-" << resultList.first << ".txt";
      std::ofstream out (ss.str ());
      for (auto& entry : resultList.second)
        {
          out << entry.first << "," << entry.second << std::endl;
        }
    }

  Simulator::Destroy ();
  return 0;
}
