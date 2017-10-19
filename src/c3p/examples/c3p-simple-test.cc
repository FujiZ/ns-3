#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/c3p-module.h"
#include "ns3/dcn-module.h"

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("C3pSimpleTest");

// attributes
DataRate link_data_rate;    //!< link capacity
DataRate btnk_data_rate;    //!< bottleneck capacity
Time link_delay;            //!< delay for each link (RTT = 6 * link_delay)
uint32_t packet_size;       //!< packet size
uint32_t queue_size;        //!< queue size for RED
uint32_t threhold;          //!< dctcp k
// c3 params
bool enable_c3;

// times
Time global_start_time;
Time global_stop_time;
Time client_start_time;
Time client_stop_time;
Time server_start_time;
Time server_stop_time;

// nodes
NodeContainer srcs;
NodeContainer routers;
NodeContainer dsts;

Ipv4InterfaceContainer dst_interfaces; //!< dst interfaces

QueueDiscContainer red_queue_discs;

std::map<uint32_t, double> tx_time;   //!< time the first packet is sent
std::map<uint32_t, double> rx_time;   //!< time the last packet is received
std::map<uint32_t, uint32_t> rx_size;   //!< total rx bytes

// throughput related
std::map<uint32_t, uint32_t> last_rx_size;   //!< rx bytes in last slot
std::map<uint32_t, std::vector<std::pair<double, double> > > throughput_list; //fId -> list<time, throughput>

void
SetupConfig (void)
{
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // RED params
  Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (packet_size));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
  Config::SetDefault ("ns3::RedQueueDisc::UseMarkP", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::MarkP", DoubleValue (2.0));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (threhold));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (threhold));
  Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (queue_size));

  // TCP params
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packet_size));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));    //!< disable delayed ack
  Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

  // C3 params
  Config::SetDefault ("ns3::c3p::C3Division::Interval", TimeValue (MilliSeconds (1)));
  Config::SetDefault ("ns3::c3p::C3Tunnel::Interval", TimeValue (MilliSeconds (1)));
  Config::SetDefault ("ns3::c3p::C3Tunnel::Gamma", DoubleValue (1.0 / 16));
  Config::SetDefault ("ns3::c3p::C3Tunnel::Rate", DataRateValue (link_data_rate));
  Config::SetDefault ("ns3::c3p::C3Tunnel::MaxRate", DataRateValue (link_data_rate));
  Config::SetDefault ("ns3::c3p::C3Tunnel::MinRate", DataRateValue (DataRate ("1Mbps")));

  // TBF params
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::DataRate", DataRateValue (link_data_rate));
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::Bucket", UintegerValue (33500));
  // Config::SetDefault ("ns3::dcn::TokenBucketFilter::Bucket", UintegerValue (83500));
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::QueueLimit", UintegerValue (queue_size));

  // enable ECN
  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));

  Config::SetDefault ("ns3::DctcpSocket::DctcpWeight", DoubleValue (1.0 / 16));
}

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
  SetupName (routers, "router");
  SetupName (dsts, "dst");
}

void
SetupTopo (uint32_t srcNum, uint32_t dstNum, const DataRate &linkBandwidth, const DataRate &btnkBandwidth, const Time &delay)
{
  NS_LOG_INFO ("Create nodes");
  srcs.Create (srcNum);
  routers.Create (2);
  dsts.Create (dstNum);

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
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (linkBandwidth));
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (delay));

  TrafficControlHelper pfifoHelper;
  uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (1000));
  pfifoHelper.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));

  NS_LOG_INFO ("Setup src nodes");
  for (auto it = srcs.Begin (); it != srcs.End (); ++it)
    {
      NetDeviceContainer devs = p2pHelper.Install (NodeContainer (*it, routers.Get (0)));
      pfifoHelper.Install (devs);
      ipv4AddrHelper.Assign (devs);
      ipv4AddrHelper.NewNetwork ();
    }
  NS_LOG_INFO ("Setup dst nodes");
  for (auto it = dsts.Begin (); it != dsts.End (); ++it)
    {
      NetDeviceContainer devs = p2pHelper.Install (NodeContainer (routers.Get (1), *it));
      pfifoHelper.Install (devs);
      dst_interfaces.Add (ipv4AddrHelper.Assign (devs).Get (1));
      ipv4AddrHelper.NewNetwork ();
    }
  NS_LOG_INFO ("Setup router nodes");
  {
    p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (btnkBandwidth));
    NetDeviceContainer devs = p2pHelper.Install (routers);
    // only backbone link has RED queue disc
    TrafficControlHelper redHelper;
    redHelper.SetRootQueueDisc ("ns3::RedQueueDisc",
                                "LinkBandwidth", DataRateValue (btnkBandwidth),
                                "LinkDelay", TimeValue (delay));
    red_queue_discs = redHelper.Install (devs);
    ipv4AddrHelper.Assign (devs);
  }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
TxTrace (uint32_t flowId, c3p::C3Tag c3Tag, Ptr<const Packet> p)
{
  FlowIdTag flowIdTag;
  flowIdTag.SetFlowId (flowId);
  p->AddPacketTag (flowIdTag);
  p->AddPacketTag (c3Tag);
  if (tx_time.find (flowId) == tx_time.end ())
    {
      tx_time[flowId] = Simulator::Now ().GetSeconds ();
    }
}

void
RxTrace (Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (packet << from);
  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  uint32_t fid = flowIdTag.GetFlowId ();
  rx_time[fid] = Simulator::Now ().GetSeconds ();
  rx_size[fid] += packet->GetSize ();
}

void
ComputeThroughput (void)
{
  // todo compute total throughput && perflow throughput
  double totalThroughput = 0.0;
  for (auto it = rx_size.begin (); it != rx_size.end (); ++it)
    {
      double cur = (it->second - last_rx_size[it->first]) * (double) 8 / 1e4; /* Convert Application RX Packets to MBits. */
      throughput_list[it->first].push_back (std::pair<double, double> (Simulator::Now ().GetSeconds (), cur));
      last_rx_size[it->first] = it->second;
      totalThroughput += cur;
    }
  throughput_list[10000].push_back (std::pair<double, double> (Simulator::Now ().GetSeconds (), totalThroughput));
  Simulator::Schedule (MilliSeconds (10), &ComputeThroughput);
}

/// cs apps
void
SetupCsServer (NodeContainer nodes, uint16_t port)
{
  PacketSinkHelper serverHelper ("ns3::L2dctSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = serverHelper.Install (nodes);
  serverApps.Start (server_start_time);
  serverApps.Stop (server_stop_time);
  for (auto it = serverApps.Begin (); it != serverApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }
}

void
SetupCsClient (Ptr<Node> node, const Address &serverAddr,
               uint32_t flowId, uint64_t flowSize, const Time &startTime)
{
  BulkSendHelper clientHelper ("ns3::L2dctSocketFactory", serverAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packet_size));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (client_stop_time);

  c3p::C3Tag c3Tag;
  c3Tag.SetTenantId (0);    /// set all tenant id to 0
  c3Tag.SetType (c3p::C3Type::CS);
  c3Tag.SetFlowSize (flowSize);
  clientApp.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId, c3Tag));
  // no need to set flow size in socket
}

/// DS apps
void
DsSocketTrace (uint64_t flowSize, Time deadline, Ptr<Socket> socket)
{
  socket->SetAttribute ("TotalBytes", UintegerValue (flowSize));
  socket->SetAttribute ("Deadline", TimeValue (deadline));
}

void
SetupDsServer (NodeContainer nodes, uint16_t port)
{
  PacketSinkHelper serverHelper ("ns3::D2tcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = serverHelper.Install (nodes);
  serverApps.Start (server_start_time);
  serverApps.Stop (server_stop_time);
  for (auto it = serverApps.Begin (); it != serverApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }
}

void
SetupDsClient (Ptr<Node> node, const Address &serverAddr,
               uint32_t flowId, uint64_t flowSize, const Time &deadline, const Time &startTime)
{
  BulkSendHelper clientHelper ("ns3::D2tcpSocketFactory", serverAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packet_size));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (client_stop_time);

  c3p::C3Tag c3Tag;
  c3Tag.SetTenantId (0);    /// set all tenant id to 0
  c3Tag.SetType (c3p::C3Type::DS);
  c3Tag.SetFlowSize (flowSize);
  c3Tag.SetDeadline (startTime + deadline);
  clientApp.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId, c3Tag));
  clientApp.Get (0)->TraceConnectWithoutContext ("SocketCreate", MakeBoundCallback (&DsSocketTrace, flowSize, deadline));
}

void
SetupApp (bool enableCS, bool enableDS, bool enableLS)
{
  if (enableCS)
    {
      // cs setup
      uint16_t port = 50000;
      SetupCsServer (dsts.Get (0), port);
      InetSocketAddress serverAddr (dst_interfaces.GetAddress (0), port);
      int KB = 1000;
      uint32_t flowId = 1000;
      for (int i = 0; i < 5; ++i)
        {
          SetupCsClient (srcs.Get (0), serverAddr, flowId++, 920 * KB, client_start_time);
        }
      for (int i = 0; i < 5; ++i)
        {
          SetupCsClient (srcs.Get (0), serverAddr, flowId++, 1380 * KB, client_start_time);
        }
    }
  if (enableDS)
    {
      // ds setup
      uint16_t port = 50001;
      SetupDsServer (dsts.Get (1), port);
      InetSocketAddress serverAddr (dst_interfaces.GetAddress (1), port);
      int KB = 1000;
      uint32_t flowId = 2000;
      for (int i = 0; i < 5; ++i)
        {
          SetupDsClient (srcs.Get (1), serverAddr, flowId++, 920 * KB, MilliSeconds (160), client_start_time);
        }
      for (int i = 0; i < 5; ++i)
        {
          SetupDsClient (srcs.Get (1), serverAddr, flowId++, 1380 * KB, MilliSeconds (200), client_start_time);
        }
    }
  if (enableLS)
    {
      ///\todo ls setup
    }
}

void
PrintRedStats (Ptr<RedQueueDisc> redQueueDisc, int index)
{
  RedQueueDisc::Stats st = redQueueDisc->GetStats ();
  std::clog << "*** RED stats from queue " << index << " ***" << std::endl;
  std::clog << "\t " << st.unforcedDrop << " drops due prob mark" << std::endl;
  std::clog << "\t " << st.unforcedMark << " marks due prob mark" << std::endl;
  std::clog << "\t " << st.forcedDrop << " drops due hard mark" << std::endl;
  std::clog << "\t " << st.qLimDrop << " drops due queue full" << std::endl;
}

void
PrintStats (void)
{
  for (auto it = red_queue_discs.Begin (); it != red_queue_discs.End (); ++it)
    {
      PrintRedStats (StaticCast<RedQueueDisc> (*it), it - red_queue_discs.Begin ());
    }
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("C3Flow", LOG_LEVEL_DEBUG);
  bool printStats = true;
  bool writeFct = false;
  bool writeThroughput = false;
  bool enableCS = false;
  bool enableDS = false;
  bool enableLS = false;
  std::string pathOut ("."); // Current directory

  link_data_rate = DataRate ("1000Mbps");
  btnk_data_rate = DataRate ("1000Mbps");
  link_delay = Time ("50us");
  packet_size = 1000;
  queue_size = 100;
  threhold = 20;
  enable_c3 = false;

  global_start_time = Seconds (0);
  global_stop_time = Seconds (10);
  server_start_time = global_start_time;
  server_stop_time = global_stop_time + Seconds (3);
  client_start_time = server_start_time + Seconds (0.2);
  client_stop_time = global_stop_time;

  CommandLine cmd;
  cmd.AddValue ("enableCS", "<0/1> enable CS test", enableCS);
  cmd.AddValue ("enableDS", "<0/1> enable DS test", enableDS);
  cmd.AddValue ("enableLS", "<0/1> enable LS test", enableLS);
  cmd.AddValue ("packetSize", "Size for every packet", packet_size);
  cmd.AddValue ("queueSize", "Queue length for RED queue", queue_size);
  cmd.AddValue ("threhold", "Threhold for RED queue", threhold);
  cmd.AddValue ("enableC3", "<0/1> enable C3 in test", enable_c3);
  cmd.AddValue ("pathOut", "Path to save results", pathOut);
  cmd.AddValue ("writeFct", "<0/1> to write FCT results", writeFct);
  cmd.AddValue ("writeThroughput", "<0/1> to write throughput results", writeThroughput);

  cmd.Parse (argc, argv);

  SetupConfig ();
  SetupTopo (3, 3, link_data_rate, btnk_data_rate, link_delay);

  DctcpSocketFactoryHelper dctcpHelper;

  // install dctcp socket factory to stack
  dctcpHelper.AddSocketFactory ("ns3::DctcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::D2tcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::L2dctSocketFactory");
  dctcpHelper.Install (srcs);
  dctcpHelper.Install (dsts);

  if (enable_c3)
    {
      dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::C3L3_5Protocol");
      l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
      l3_5Helper.Install(srcs);
      l3_5Helper.Install (dsts);
      c3p::C3Division::AddDivisionType (c3p::C3Type::CS, "ns3::c3p::C3CsDivision");
      c3p::C3Division::AddDivisionType (c3p::C3Type::DS, "ns3::c3p::C3DsDivision");
      // create division : one division just for test
      if (enableCS)
        {
          Ptr<c3p::C3Division> division = c3p::C3Division::CreateDivision (0, c3p::C3Type::CS);
          division->SetAttribute ("Weight", DoubleValue (0.2));
        }
      if (enableDS)
        {
          Ptr<c3p::C3Division> division = c3p::C3Division::CreateDivision (0, c3p::C3Type::DS);
          division->SetAttribute ("Weight", DoubleValue (0.2));
        }
    }

  SetupApp (enableCS, enableDS, enableLS);

  if (writeThroughput)
    {
      Simulator::Schedule (server_start_time, &ComputeThroughput);
    }

  Simulator::Stop (server_stop_time);
  Simulator::Run ();

  if (printStats)
    {
      PrintStats ();
    }
  // output cs result(FCT)
  if (writeFct)
    {
      std::stringstream ss;
      ss << pathOut << "/result.txt";
      std::ofstream out (ss.str ());
      for (auto& entry : tx_time)
        {
          out << entry.first
              << "," << entry.second
              << "," << rx_time[entry.first]
              << "," << rx_size[entry.first]
              << std::endl;
        }
    }

  if (writeThroughput)
    {
      for (auto& resultList : throughput_list)
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
