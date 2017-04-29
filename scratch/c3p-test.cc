#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dcn-module.h"

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("C3pTest");

// attributes
DataRate link_data_rate;    //!< bottleneck capacity
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

  // TCP params
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packet_size));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));    //!< disable delayed ack

  // C3 params
  Config::SetDefault ("ns3::dcn::C3Division::Interval", TimeValue (MilliSeconds (3)));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Interval", TimeValue (MilliSeconds (1)));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Gamma", DoubleValue (0.75));
  Config::SetDefault ("ns3::dcn::C3Tunnel::DataRate", DataRateValue (link_data_rate));

  // TBF params
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::DataRate", DataRateValue (link_data_rate));
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::Bucket", UintegerValue (33500));
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
SetupTopo (uint32_t srcNum, uint32_t dstNum, const DataRate& bandwidth, const Time& delay)
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
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (bandwidth));
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
    NetDeviceContainer devs = p2pHelper.Install (routers);
    // only backbone link has RED queue disc
    TrafficControlHelper redHelper;
    redHelper.SetRootQueueDisc ("ns3::RedQueueDisc",
                                "LinkBandwidth", DataRateValue (bandwidth),
                                "LinkDelay", TimeValue (delay));
    red_queue_discs = redHelper.Install (devs);
    ipv4AddrHelper.Assign (devs);
  }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
CsTxTrace (uint32_t flowId, dcn::C3Tag c3Tag, Ptr<const Packet> p)
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
CsRxTrace (Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (packet << from);
  FlowIdTag flowIdTag;
  bool retval = packet->PeekPacketTag (flowIdTag);
  NS_ASSERT (retval);
  rx_time[flowIdTag.GetFlowId ()] = Simulator::Now ().GetSeconds ();
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << ": " << flowIdTag.GetFlowId () << " received " << packet->GetSize ());
}

void
SetupCsServer (NodeContainer nodes, uint16_t port)
{
  PacketSinkHelper serverHelper ("ns3::L2dctSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = serverHelper.Install (nodes);
  serverApps.Start (server_start_time);
  serverApps.Stop (server_stop_time);
  for (auto it = serverApps.Begin (); it != serverApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&CsRxTrace));
    }
}

void
SetupCsClient (Ptr<Node> node, const Address &serverAddr,
               uint32_t flowId, const Time &startTime, uint64_t flowSize)
{
  BulkSendHelper clientHelper ("ns3::L2dctSocketFactory", serverAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packet_size));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (client_stop_time);
  dcn::C3Tag c3Tag;
  c3Tag.SetType (dcn::C3Type::CS);
  c3Tag.SetFlowSize (flowSize);
  c3Tag.SetTenantId (0);    /// set all tenant id to 0
  clientApp.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&CsTxTrace, flowId, c3Tag));
  // no need to set flow size in socket
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
      for (int i = 0; i < 5; ++i)
        {
          SetupCsClient (srcs.Get (0), serverAddr, FlowIdTag::AllocateFlowId (), client_start_time, 92 * KB);
          SetupCsClient (srcs.Get (0), serverAddr, FlowIdTag::AllocateFlowId (), client_start_time, 138 * KB);
        }
    }
  if (enableDS)
    {
      ///\todo ds setup
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
  std::cout << "*** RED stats from queue " << index << " ***" << std::endl;
  std::cout << "\t " << st.unforcedDrop << " drops due prob mark" << std::endl;
  std::cout << "\t " << st.unforcedMark << " marks due prob mark" << std::endl;
  std::cout << "\t " << st.forcedDrop << " drops due hard mark" << std::endl;
  std::cout << "\t " << st.qLimDrop << " drops due queue full" << std::endl;
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
  bool printStats = true;
  bool writeFct = false;
  bool enableCS = false;
  bool enableDS = false;
  bool enableLS = false;
  std::string pathOut ("."); // Current directory

  link_data_rate = DataRate ("500Mbps");
  link_delay = Time ("50us");
  packet_size = 1024;
  queue_size = 250;
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

  cmd.Parse (argc, argv);

  SetupConfig ();
  SetupTopo (3, 3, link_data_rate, link_delay);

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
      dcn::C3Division::AddDivisionType (dcn::C3Type::CS, "ns3::dcn::C3CsDivision");
      // create division : one division just for test
      dcn::C3Division::CreateDivision (0, dcn::C3Type::CS);
    }

  SetupApp (enableCS, enableDS, enableLS);

  Simulator::Stop (server_stop_time);
  Simulator::Run ();

  if (printStats)
    {
      PrintStats ();
    }
  ///\todo output cs result(FCT)
  if (writeFct)
    {
      std::stringstream ss;
      ss << pathOut << "/fct.txt";
      std::ofstream out (ss.str ());
      for (auto& entry : tx_time)
        {
          out << entry.first << "," << entry.second
              << "," << rx_time[entry.first] << std::endl;
        }
    }

  Simulator::Destroy ();

  return 0;
}
