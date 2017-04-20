#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("D2tcpTest");

// attributes
DataRate link_data_rate;
Time link_delay;
bool use_d2tcp;

// queue params
uint32_t packet_size;
uint32_t queue_size;
uint32_t threhold;

// The times
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

// dst interfaces
Ipv4InterfaceContainer dst_interfaces;

// receive status
std::map<uint32_t, uint64_t> totalRx;   //fId->receivedBytes
std::map<uint32_t, uint64_t> lastRx;
// throughput result
std::map<uint32_t, std::vector<std::pair<double, double> > > throughputResult; //fId -> list<time, throughput>

void
SocketCreateTrace (uint64_t flowSize, Time deadline, Ptr<Socket> socket)
{
  socket->SetAttribute ("TotalBytes", UintegerValue (flowSize));
  socket->SetAttribute ("Deadline", TimeValue (deadline));
}

void
TxTrace (uint32_t flowId, Ptr<const Packet> p)
{
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
  NS_LOG_DEBUG (Simulator::Now () << ", " << flowIdTag.GetFlowId () << ", " << totalRx[flowIdTag.GetFlowId ()]);
}

void
CalculateThroughput (void)
{
  for (auto it = totalRx.begin (); it != totalRx.end (); ++it)
    {
      double cur = (it->second - lastRx[it->first]) * (double) 8/1e5; /* Convert Application RX Packets to MBits. */
      throughputResult[it->first].push_back (std::pair<double, double> (Simulator::Now ().GetSeconds (), cur));
      lastRx[it->first] = it->second;
    }
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
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
SetupTopo (uint32_t srcCount, uint32_t dstCount, const DataRate& bandwidth, const Time& delay)
{
  NS_LOG_INFO ("Create nodes");
  srcs.Create (srcCount);
  routers.Create (2);
  dsts.Create (dstCount);

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
    redHelper.Install (devs);
    ipv4AddrHelper.Assign (devs);
  }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
SetupDsServer (NodeContainer nodes, uint16_t port)
{
  PacketSinkHelper serverHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = serverHelper.Install (nodes);
  serverApps.Start (server_start_time);
  serverApps.Stop (server_stop_time);
  for (auto it = serverApps.Begin (); it != serverApps.End (); ++it)
    {
      (*it)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxTrace));
    }
}


void
SetupDsClient (NodeContainer nodes, const Address &serverAddr,
               const Time &startTime, uint64_t flowSize, const Time &deadline)
{
  static uint32_t index = 1;
  BulkSendHelper clientHelper ("ns3::TcpSocketFactory", serverAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  ApplicationContainer clientApps = clientHelper.Install (nodes);
  clientApps.Start (startTime);
  clientApps.Stop (client_stop_time);
  for(auto it = clientApps.Begin (); it != clientApps.End (); ++it)
    {
      // trace tx
      (*it)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, index++));
      // trace socket create
      if (use_d2tcp)
        {
          (*it)->TraceConnectWithoutContext ("SocketCreate", MakeBoundCallback (&SocketCreateTrace, flowSize, deadline));
        }
    }
}

void
SetupApp (bool enableLS, bool enableDS, bool enableCS)
{
  if (enableLS)
    {
      ///\todo ls setup
    }
  if (enableDS)
    {
      uint16_t port = 50000;
      uint64_t MB = 1024 * 1024;
      SetupDsServer (dsts.Get (0), port);
      InetSocketAddress serverAddr (dst_interfaces.GetAddress (0), port);
      SetupDsClient (srcs.Get (0), serverAddr, client_start_time, 150 * MB, Time ("3000ms"));
      SetupDsClient (srcs.Get (1), serverAddr, client_start_time, 220 * MB, Time ("8000ms"));
      SetupDsClient (srcs.Get (2), serverAddr, client_start_time, 350 * MB, Time ("50000ms"));
      SetupDsClient (srcs.Get (3), serverAddr, client_start_time, 500 * MB, Time ("60000ms"));
    }
  if (enableCS)
    {
      ///\todo cs setup
    }
}

void
SetupConfig (void)
{
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  // RED params
  NS_LOG_INFO ("Set RED params");
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
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));

  // enable ECN
  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));

  Config::SetDefault ("ns3::DctcpSocket::DctcpWeight", DoubleValue (1.0 / 16));

  if (use_d2tcp)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketBaseType", TypeIdValue(TypeId::LookupByName ("ns3::D2tcpSocket")));
    }
  else
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketBaseType", TypeIdValue(TypeId::LookupByName ("ns3::DctcpSocket")));
    }
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("D2tcpSocket", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("C3pTest", LOG_LEVEL_DEBUG);
  bool writeThroughput = false;
  std::string pathOut ("."); // Current directory

  global_start_time = MilliSeconds (0);
  global_stop_time = Seconds (15);
  server_start_time = global_start_time;
  server_stop_time = global_stop_time + Seconds (3);
  client_start_time = server_start_time + Seconds (0.2);
  client_stop_time = global_stop_time;

  link_data_rate = DataRate ("1000Mbps");
  link_delay = Time ("50us");
  packet_size = 1024;
  queue_size = 250;
  threhold = 20;

  use_d2tcp = false;

  CommandLine cmd;
  cmd.AddValue ("useD2tcp", "<0/1> to use d2tcp in test", use_d2tcp);
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeThroughput", "<0/1> to write throughtput results", writeThroughput);

  cmd.Parse (argc, argv);

  SetupConfig ();
  SetupTopo (10, 1, link_data_rate, link_delay);
  SetupApp (false, true, false);

  if (writeThroughput)
    {
      Simulator::Schedule (server_start_time, &CalculateThroughput);
    }

  Simulator::Stop (server_stop_time);
  Simulator::Run ();

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
