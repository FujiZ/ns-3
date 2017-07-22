#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dcn-module.h"

#include <sstream>
#include <map>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AddcnConvergenceTest");

uint32_t checkTimes;
double avgQueueSize;

// attributes
std::string non_btnk_rate;
std::string link_data_rate;
std::string link_delay;
uint32_t packet_size;
uint32_t queue_size;
uint32_t threshold;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;
double client_interval_time;

// nodes
NodeContainer nodes;
NodeContainer clients;
NodeContainer switchs;
NodeContainer servers;

// server interfaces
Ipv4InterfaceContainer serverInterfaces;

// receive status
std::map<uint32_t, uint64_t> totalRx;   //fId->receivedBytes
std::map<uint32_t, uint64_t> lastRx;

uint32_t FlowIdBaseA=100000;
uint32_t FlowIdBaseB=200000;
// throughput result
std::map<uint32_t, std::vector<std::pair<double, double> > > throughputResult; //fId -> list<time, throughput>

QueueDiscContainer queueDiscs;

std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;

void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = StaticCast<RedQueueDisc> (queue)->GetQueueSize ();

  avgQueueSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();

  std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close ();
}

void
TxTrace (uint32_t flowId, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (flowId << p);
  FlowIdTag flowIdTag;
  flowIdTag.SetFlowId (flowId);
  p->AddByteTag (flowIdTag);
}

void
RxTrace (Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (packet << from);
  FlowIdTag flowIdTag;
  //bool retval = packet->FindFirstMatchingByteTag (flowIdTag);
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

Ipv4InterfaceContainer
MakeLink (const std::string &type, Ptr<Node> src, Ptr<Node> dst,
          DataRate bw, Time delay, Ipv4AddressHelper &ipv4AddrHelper)
{
  PointToPointHelper p2pHelper;
  p2pHelper.SetQueue ("ns3::DropTailQueue");
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (delay));
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (bw));
  NetDeviceContainer devs = p2pHelper.Install (src, dst);

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
  queueDiscs = trafficHelper.Install (devs);
  auto interfaces = ipv4AddrHelper.Assign (devs);
  ipv4AddrHelper.NewNetwork ();
  return interfaces;
}


void
BuildTopo (uint32_t clientNo, uint32_t serverNo, DataRate host2rout, DataRate rout2rout, Time linkDelay)
{
  NS_LOG_INFO ("Create nodes");
  clients.Create (clientNo);
  switchs.Create (2);
  servers.Create (serverNo);

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (clients);
  internet.Install (switchs);
  internet.Install (servers);

  dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::ADDCNL3_5Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::UdpL4Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
  l3_5Helper.Install(clients);
  l3_5Helper.Install(servers);


  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  for (auto it = clients.Begin (); it != clients.End (); ++it)
    {
      MakeLink("pfifo", *it, switchs.Get(0), host2rout, linkDelay, ipv4);
    }


  for (auto it = servers.Begin (); it != servers.End (); ++it)
    {
      auto interfaces = MakeLink("pfifo", switchs.Get(1), *it, host2rout, linkDelay, ipv4);
      serverInterfaces.Add (interfaces.Get (1));
    }

  MakeLink ("red", switchs.Get (0), switchs.Get(1), rout2rout, linkDelay, ipv4);
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
InstallSink (NodeContainer nodes, uint16_t port, const Time &startTime, const Time &stopTime)
{
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sinkHelper.Install (nodes);
  sinkApps.Start (startTime);
  sinkApps.Stop (stopTime);
  for (auto it = sinkApps.Begin (); it != sinkApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }
}

//Client App
void
InstallLsClient (Ptr<Node> node, const Address &sinkAddr, uint32_t tenantId,
                 uint32_t flowId, uint64_t flowSize, uint64_t packetSize,
                 const Time &startTime, const Time &stopTime)
{
  dcn::AddcnBulkSendHelper clientHelper ("ns3::TcpSocketFactory", sinkAddr);
  //clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  //clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  auto it = clientApp.Begin();
  Ptr<dcn::AddcnBulkSendApplication> app = StaticCast<dcn::AddcnBulkSendApplication> (*it);

  app->SetTenantId (tenantId);
  //app->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId));
  //app->SetFlowType (dcn::C3Type::LS);
  app->SetFlowId (flowId);
  app->SetFlowSize (flowSize);
  app->SetSegSize (packetSize);
}

void
BuildAppsTest (void)
{
  // SINK is in the right side
  uint16_t port = 50000;
  InstallSink (servers, port, Seconds (sink_start_time), Seconds (sink_stop_time));

  // set different start/stop time for each app
  double clientStartTime = client_start_time;
  double clientStopTime = client_stop_time;
  //uint32_t i = 0;
  //for (auto it = clients.Begin (); it != clients.End (); ++it)
  for (int i = 0; i < 3; i++)
  {
    auto it = clients.Begin ();
    InstallLsClient(*it, InetSocketAddress (serverInterfaces.GetAddress (0), port), i, i+1, 0, 
                    packet_size, Seconds(clientStartTime), Seconds (clientStopTime));
    clientStartTime += client_interval_time;
    //i++;
  }

  for (int i = 0; i < 3; i++)
  {
    Ptr<ns3::dcn::ADDCNSlice> sliceA = ns3::dcn::ADDCNSlice::GetSlice(i, dcn::C3Type::DS);
    sliceA->SetScale((i + 1.0)/4.0);
  }
}

void
SetConfig (bool useEcn, bool useDctcp)
{

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  // RED params
  NS_LOG_INFO ("Set RED params");
  Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (packet_size));
  Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (threshold));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (2*threshold));
  Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (queue_size));
  Config::SetDefault ("ns3::RedQueueDisc::UseMarkP", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::MarkP", DoubleValue (2.0));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true)); // Should always be true

  // TCP params
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packet_size));
  // Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  if (useDctcp)
    {
      Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
      Config::SetDefault ("ns3::TcpL4Protocol::SocketBaseType", TypeIdValue(TypeId::LookupByName ("ns3::dcn::DctcpSocket")));
      Config::SetDefault ("ns3::dcn::DctcpSocket::DctcpWeight", DoubleValue (1.0 / 16));
    }
  if (useEcn)
    {
      Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
    }
}

int
main (int argc, char *argv[])
{

  //LogComponentEnable ("DctcpSocket", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("ADDCNExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNL3_5Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNSlice", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNFlow", LOG_LEVEL_ALL);
  //LogComponentEnable ("TcpSocketBase", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("TcpCongestionOps", LOG_LEVEL_ALL);
  
  bool useEcn = false;
  bool useDctcp = false; 
  std::string pathOut;
  bool writeForPlot = true;
  bool writePcap = false;
  bool flowMonitor = false;
  bool writeThroughput = true;

  bool printRedStats = true;

  global_start_time = 0.0;
  global_stop_time = 10.0;
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 1.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 1.0;
  client_interval_time = 5.0;

  non_btnk_rate = "10Gbps";
  link_data_rate = "200Mbps";
  link_delay = "1ms";
  queue_size = 100;
  packet_size = 500;
  threshold = 20;

  int size_tenantA = 5;
  int size_tenantB = 5;
  int dumbell_host_num = 3;

  double weight_tenantA = 0.5;
  double weight_tenantB = 0.5;

  // Will only save in the directory if enable opts below
  pathOut = "."; // Current directory
  CommandLine cmd;
  cmd.AddValue ("useEcn", "<0/1> to use ecn in test", useEcn);
  cmd.AddValue ("useDctcp", "<0/1> to use dctcp in test", useDctcp);
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);
  cmd.AddValue ("writeThroughput", "<0/1> to write throughtput results", writeThroughput);
  cmd.AddValue ("sizeTenantA", "Flow number of tenant A", size_tenantA);
  cmd.AddValue ("sizeTenantB", "Flow number of tenant B", size_tenantB);
  cmd.AddValue ("weightTenantA", "Global weight for tenant A", weight_tenantA);

  cmd.Parse (argc, argv);

  totalRx[0] = 0;
  totalRx[1] = 0;
  lastRx[0] = 0;
  lastRx[1] = 0;

  weight_tenantB = 1 - weight_tenantA;
  double scale = (weight_tenantA * size_tenantB) / (weight_tenantB * size_tenantA);
  if (scale < 0.1 || scale > 10.0)
  {
    printf("Illegal size & weight");
    return -1;
  }

  double scaleA = 1;
  double scaleB = 1;

  if (scale > 1)
  {
    scaleA = 1.0;
    scaleB = 1.0 / scale;
  }
  else
  {
    scaleA = scale;
    scaleB = 1.0;
  }

  SetConfig (useEcn, useDctcp);
  BuildTopo (dumbell_host_num, dumbell_host_num, DataRate(non_btnk_rate), DataRate(link_data_rate), Time(link_delay));

  Ptr<ns3::dcn::ADDCNSlice> sliceA = ns3::dcn::ADDCNSlice::GetSlice(0, dcn::C3Type::DS);
  Ptr<ns3::dcn::ADDCNSlice> sliceB = ns3::dcn::ADDCNSlice::GetSlice(1, dcn::C3Type::DS);

  sliceA->SetScale(scaleA);
  sliceB->SetScale(scaleB);

  uint16_t sink_port = 50000;
  InstallSink (servers, sink_port, Seconds (sink_start_time), Seconds (sink_stop_time));

/*
  auto it = clients.Begin ();
  InstallLsClient(*it, InetSocketAddress (serverInterfaces.GetAddress (0), sink_port), 0, 100000, 0, 
                  packet_size, Seconds(client_start_time), Seconds (client_stop_time));
  InstallLsClient(*it, InetSocketAddress (serverInterfaces.GetAddress (0), sink_port), 1, 200000, 0, 
                  packet_size, Seconds(client_start_time), Seconds (client_stop_time));
  InstallLsClient(*it, InetSocketAddress (serverInterfaces.GetAddress (0), sink_port), 1, 200001, 0, 
                  packet_size, Seconds(client_start_time), Seconds (client_stop_time));
*/

  auto hostStream = CreateObject<UniformRandomVariable> ();
  hostStream->SetStream (2);
  hostStream->SetAttribute ("Min", DoubleValue (0));
  hostStream->SetAttribute ("Max", DoubleValue (servers.GetN () - 1));

  for (int i = 0; i < size_tenantA; i++)
    {
      uint32_t srcIndex = hostStream->GetInteger ();
      uint32_t dstIndex = hostStream->GetInteger ();
      uint32_t flowId = FlowIdBaseA++;
      InstallLsClient (clients.Get (srcIndex), InetSocketAddress (serverInterfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, 0, packet_size, Seconds (client_start_time), Seconds (client_stop_time));
    }

  for (int i = 0; i < size_tenantB; i++)
    {
      uint32_t srcIndex = hostStream->GetInteger ();
      uint32_t dstIndex = hostStream->GetInteger ();
      uint32_t flowId = FlowIdBaseB++;
      InstallLsClient (clients.Get (srcIndex), InetSocketAddress (serverInterfaces.GetAddress (dstIndex), sink_port), 1,
                           flowId, 0, packet_size, Seconds (client_start_time), Seconds (client_stop_time));
    }

  //BuildAppsTest ();

  //ns3::dcn::ADDCNSlice::SetInterval(MilliSeconds (100));
  //ns3::dcn::ADDCNSlice::Start(Seconds(global_start_time));
  //ns3::dcn::ADDCNSlice::Stop(Seconds(global_stop_time));

  if (writePcap)
    {
      PointToPointHelper ptp;
      std::stringstream stmp;
      stmp << pathOut << "/dctcp";
      ptp.EnablePcapAll (stmp.str ());
    }

  Ptr<FlowMonitor> flowmon;
  if (flowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  if (writeForPlot)
    {
      filePlotQueue << pathOut << "/" << "red-queue.plotme";
      filePlotQueueAvg << pathOut << "/" << "red-queue_avg.plotme";

      remove (filePlotQueue.str ().c_str ());
      remove (filePlotQueueAvg.str ().c_str ());
      Ptr<QueueDisc> queue = queueDiscs.Get (0);
      Simulator::ScheduleNow (&CheckQueueSize, queue);
    }

  if (writeThroughput)
    {
      Simulator::Schedule (Seconds (sink_start_time), &CalculateThroughput);
    }

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  if (flowMonitor)
    {
      std::stringstream stmp;
      stmp << pathOut << "/red.flowmon";

      flowmon->SerializeToXmlFile (stmp.str (), false, false);
    }

  if (printRedStats)
    {
      RedQueueDisc::Stats st = StaticCast<RedQueueDisc> (queueDiscs.Get (0))->GetStats ();
      std::cout << "*** RED stats from Node 2 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due prob mark" << std::endl;
      std::cout << "\t " << st.unforcedMark << " marks due prob mark" << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due hard mark" << std::endl;
      std::cout << "\t " << st.qLimDrop << " drops due queue full" << std::endl;

      st = StaticCast<RedQueueDisc> (queueDiscs.Get (1))->GetStats ();
      std::cout << "*** RED stats from Node 3 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due prob mark" << std::endl;
      std::cout << "\t " << st.unforcedMark << " marks due prob mark" << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due hard mark" << std::endl;
      std::cout << "\t " << st.qLimDrop << " drops due queue full" << std::endl;
    }

  if (writeThroughput)
    {
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
    }
  Simulator::Destroy ();

  return 0;
}
