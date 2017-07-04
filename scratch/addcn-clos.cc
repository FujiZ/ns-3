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

NS_LOG_COMPONENT_DEFINE ("AddcnCLOS");

/*

Topology:

spine switch          s3.0           s3.1



aggre switch   s2.0    s2.1         s2.2    s2.3


TOR   switch   s1.0    s1.1         s1.2    s1.3


servers      s0   s1  s2   s3     s4   s5  s6   s7  

*/

uint32_t checkTimes;
double avgQueueSize;

// attributes
std::string link_rate_tor2srv;
std::string link_rate_agg2tor;
std::string link_rate_spi2agg;
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
NodeContainer servers;
NodeContainer torswch;
NodeContainer aggswch;
NodeContainer spiswch;

// server interfaces
Ipv4InterfaceContainer serverInterfaces;

// receive status
std::map<uint32_t, uint64_t> totalRx;   //fId->receivedBytes
std::map<uint32_t, uint64_t> lastRx;

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
  bool retval = packet->FindFirstMatchingByteTag (flowIdTag);
  NS_ASSERT (retval);
  if (totalRx.find (flowIdTag.GetFlowId ()) != totalRx.end ())
    {
      totalRx[flowIdTag.GetFlowId ()] += packet->GetSize ();
    }
  else
    {
      totalRx[flowIdTag.GetFlowId ()] = packet->GetSize ();
      lastRx[flowIdTag.GetFlowId ()] = 0;
    }
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
SetName (void)
{
  // add name to clients
  int i = 0;
  for(auto it = servers.Begin (); it != servers.End (); ++it)
    {
      std::stringstream ss;
      ss << "SRV" << i++;
      Names::Add (ss.str (), *it);
    }
  i = 0;
  for(auto it = torswch.Begin (); it != torswch.End (); ++it)
    {
      std::stringstream ss;
      ss << "TOR" << i++;
      Names::Add (ss.str (), *it);
    }
  i = 0;
  for(auto it = aggswch.Begin (); it != aggswch.End (); ++it)
    {
      std::stringstream ss;
      ss << "AGG" << i++;
      Names::Add (ss.str (), *it);
    }
  i = 0;
  for(auto it = spiswch.Begin (); it != spiswch.End (); ++it)
    {
      std::stringstream ss;
      ss << "SPIN" << i++;
      Names::Add (ss.str (), *it);
    }
}

void
BuildTopo (uint32_t srvNum, uint32_t torNum, uint32_t aggNum, uint32_t spiNum)
{
  NS_LOG_INFO ("Create nodes");
  servers.Create (srvNum);
  torswch.Create (torNum);
  aggswch.Create (aggNum);
  spiswch.Create (spiNum);

  SetName ();

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (servers);
  internet.Install (torswch);
  internet.Install (aggswch);
  internet.Install (spiswch);

  dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::ADDCNL3_5Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::UdpL4Protocol");
  l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
  l3_5Helper.Install(servers);

  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");//, "Limit", UintegerValue (90));
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue");//, "MaxPackets", UintegerValue (90));

  TrafficControlHelper tchRed;
  tchRed.SetRootQueueDisc ("ns3::RedQueueDisc", "LinkBandwidth", StringValue (link_rate_spi2agg),
                           "LinkDelay", StringValue (link_delay));

  NS_LOG_INFO ("Create channels");
  PointToPointHelper tor2srv, agg2tor, spi2agg;

  tor2srv.SetQueue ("ns3::DropTailQueue");
  tor2srv.SetDeviceAttribute ("DataRate", StringValue (link_rate_tor2srv));
  tor2srv.SetChannelAttribute ("Delay", StringValue (link_delay));

  agg2tor.SetQueue ("ns3::DropTailQueue");
  agg2tor.SetDeviceAttribute ("DataRate", StringValue (link_rate_agg2tor));
  agg2tor.SetChannelAttribute ("Delay", StringValue (link_delay));

  spi2agg.SetQueue ("ns3::DropTailQueue");
  spi2agg.SetDeviceAttribute ("DataRate", StringValue (link_rate_spi2agg));
  spi2agg.SetChannelAttribute ("Delay", StringValue (link_delay));

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  int i = 0;
  for (auto it = torswch.Begin (); it != torswch.End (); ++it)
    {
      NetDeviceContainer left = tor2srv.Install (NodeContainer (*it, servers.Get (2 * i)));
      NetDeviceContainer right = tor2srv.Install (NodeContainer (*it, servers.Get (2 * i + 1)));
      serverInterfaces.Add (ipv4.Assign (left).Get (1));
      ipv4.NewNetwork ();
      serverInterfaces.Add (ipv4.Assign (right).Get (1));
      ipv4.NewNetwork ();
      tchPfifo.Install (left);
      tchPfifo.Install (right);
      i++;
    }
  i = 0;
  int t;
  for (auto it = aggswch.Begin (); it != aggswch.End (); ++it)
    {
      t = i / 2;
      NetDeviceContainer left = agg2tor.Install (NodeContainer (*it, torswch.Get (2 * t)));
      NetDeviceContainer right = agg2tor.Install (NodeContainer (*it, torswch.Get (2 * t + 1)));
      ipv4.Assign (left);
      ipv4.NewNetwork ();
      ipv4.Assign (right);
      ipv4.NewNetwork ();
      tchPfifo.Install (left);
      tchPfifo.Install (right);
      i++;
    }

  for (auto it = spiswch.Begin (); it != spiswch.End (); ++it)
    {
      for (auto agg = aggswch.Begin (); agg != aggswch.End (); ++agg)
       {
         NetDeviceContainer devs = spi2agg.Install (NodeContainer (*it, *agg));
         ipv4.Assign (devs);
         ipv4.NewNetwork ();
         tchRed.Install (devs); // RED in the core
       }
    }
  /*
  {
    p2p.SetQueue ("ns3::DropTailQueue");
    p2p.SetDeviceAttribute ("DataRate", StringValue (link_data_rate));
    p2p.SetChannelAttribute ("Delay", StringValue (link_delay));
    NetDeviceContainer devs = p2p.Install (switchs);
    // only backbone link has RED queue disc
    queueDiscs = tchRed.Install (devs);
    ipv4.Assign (devs);
  }
  */
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
BuildAppsTest (void)
{
  // SINK is in the right rack
  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  // for (int i = 4; i < 8; i++)
  ApplicationContainer sinkApps = sinkHelper.Install (servers);
  sinkApps.Start (Seconds (sink_start_time));
  sinkApps.Stop (Seconds (sink_stop_time));
  for (auto it = sinkApps.Begin (); it != sinkApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }

  // Connection one
  // Clients are in left rack 
  /*
   * Create the OnOff applications to send TCP to the server
   * onoffhelper is a client that send data to TCP destination
   */
  
  //OnOffHelper clientHelper ("ns3::TcpSocketFactory", InetSocketAddress (serverInterfaces.GetAddress (0), port));
  //clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  //clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  //clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate (link_data_rate)));
  // clientHelper.SetAttribute ("PacketSize", UintegerValue (1000));

  double clientStartTime = client_start_time;
  double clientStopTime = client_stop_time;
  //int i = 3;
  for (int i = 0; i < 3; i++)
    {
      dcn::AddcnBulkSendHelper clientHelper ("ns3::TcpSocketFactory", InetSocketAddress (serverInterfaces.GetAddress (i + 4), port));
      ApplicationContainer clientApps = clientHelper.Install (servers.Get (i));
      auto it = clientApps.Begin();
      Ptr<dcn::AddcnBulkSendApplication> app = StaticCast<dcn::AddcnBulkSendApplication> (*it);
      app->SetStartTime (Seconds (clientStartTime));
      app->SetStopTime (Seconds (clientStopTime));
      app->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, i));
      app->SetFlowSize (0);
      app->SetTenantId (i);
      app->SetSegSize (packet_size);
      //clientStartTime += client_interval_time;
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
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
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
  // LogComponentEnable ("ADDCNExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNL3_5Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNSlice", LOG_LEVEL_ALL);
  //LogComponentEnable ("ADDCNFlow", LOG_LEVEL_ALL);
  //LogComponentEnable ("TcpSocketBase", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("TcpCongestionOps", LOG_LEVEL_ALL);
  
  bool useEcn = false;
  bool useDctcp = false; 
  bool useECMP = false;
  bool writeForPlot = false;
  bool writePcap = false;
  bool flowMonitor = false;
  bool writeThroughput = true;
  bool printRedStats = false;

  std::string pathOut;

  global_start_time = 0.0;
  global_stop_time = 80.0;
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 3.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 1.0;
  client_interval_time = 10.0;

  link_rate_spi2agg = "200Mbps";
  link_rate_agg2tor = "500Mbps";
  link_rate_tor2srv = "500Mbps";
  link_delay = "500us";
  queue_size = 100;
  packet_size = 500;
  threshold = 20;

  // Will only save in the directory if enable opts below
  pathOut = "."; // Current directory
  CommandLine cmd;
  cmd.AddValue ("useEcn", "<0/1> to use ecn in test", useEcn);
  cmd.AddValue ("useDctcp", "<0/1> to use dctcp in test", useDctcp);
  cmd.AddValue ("useECMP", "<0/1> to use ecmp in test", useECMP);
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);
  cmd.AddValue ("writeThroughput", "<0/1> to write throughtput results", writeThroughput);

  cmd.Parse (argc, argv);

  if (useECMP)
    {
      //Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_HASH"));
      //Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_RANDOM"));
      Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_FLOWCELL"));
    }

  SetConfig (useEcn, useDctcp);
  BuildTopo (8, 4, 4, 2);
  BuildAppsTest ();

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
