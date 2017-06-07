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

/*

Topology:

s0 - sw0        sw1 - d0
       \       /
        r0 - r1
       /       \
s1 - sw2        sw3 - d1
       \       /
        r2 - r3

*/
NS_LOG_COMPONENT_DEFINE ("C3pECMP");

uint32_t ds_flowid_base = 100000;
uint32_t cs_flowid_base = 500000;

// topo attributes
uint32_t src_num = 2;
uint32_t dst_num = 2;
Time link_delay ("0.050ms");
DataRate btnk_bw ("1Gbps");
DataRate edge_bw ("10Gbps");
uint32_t queue_size = 100;
uint32_t threhold = 20;

// c3 attributes
Time tunnel_interval ("1ms");
Time division_interval ("3ms");
DataRate tunnel_bw ("500Mbps"); //!< initial tunnel bw

// time attributes
double sim_time = 15;

// attributes
uint32_t workload = 0;
double mice_load = 0.8;
uint32_t packet_size = 1000;
uint32_t min_flow_size = 1;
uint32_t max_flow_size = 666667;
uint32_t av_flow_size = 5116;
uint16_t sink_port = 50000;
int64_t random_seed = 2;

NodeContainer srcs;
NodeContainer dsts;
NodeContainer switchs;
NodeContainer routers;
Ipv4InterfaceContainer src_interfaces;    //!< hosts interfaces include address
Ipv4InterfaceContainer dst_interfaces;    //!< hosts interfaces include address

std::map<uint32_t, double> tx_time;     //!< time the first packet is sent
std::map<uint32_t, double> rx_time;     //!< time the last packet is received
std::map<uint32_t, uint64_t> rx_size;   //!< total rx bytes
std::map<uint32_t, uint64_t> rx_node;   //!< total rx bytes
std::map<uint32_t, uint64_t> last_rx;   //!< last total rx bytes
                                        //!< fId -> list<time, throughput>
std::map<uint32_t, std::vector<std::pair<double, double>>> goodput_result; 

void
CalculateThroughput (void)
{
  for (auto it = rx_node.begin (); it != rx_node.end (); ++it)
    {
      double cur = (it->second - last_rx[it->first]) * (double) 8/1e8; /* Convert Application RX Packets to GBits. */
      goodput_result[it->first].push_back (std::pair<double, double> (Simulator::Now ().GetSeconds (), cur));
      last_rx[it->first] = it->second;
    }
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
SetupConfig (void)
{
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // ECMP params
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue (true));

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

  // C3 params
  Config::SetDefault ("ns3::dcn::C3Division::Interval", TimeValue (division_interval));
  Config::SetDefault ("ns3::dcn::C3Division::RateThresh", DataRateValue (btnk_bw));

  Config::SetDefault ("ns3::dcn::C3Tunnel::Interval", TimeValue (tunnel_interval));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Gamma", DoubleValue (1.0 / 16));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Rate", DataRateValue (tunnel_bw));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MaxRate", DataRateValue (btnk_bw));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MinRate", DataRateValue (DataRate ("1Mbps")));
  Config::SetDefault ("ns3::dcn::C3Tunnel::RateThresh", DataRateValue (tunnel_bw));
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
SetupTopo (uint32_t srcNum, uint32_t dstNum, DataRate edgeBW, DataRate switchBW, Time linkDelay)
{
  InternetStackHelper internetHelper;
  Ipv4AddressHelper  ipv4AddrHelper ("10.1.1.0", "255.255.255.0");

  srcs.Create (srcNum);
  routers.Create (2 * srcNum);
  switchs.Create (2 * srcNum);
  dsts.Create (dstNum);
  internetHelper.Install (srcs);
  internetHelper.Install (dsts);
  internetHelper.Install (routers);
  internetHelper.Install (switchs);

  Ipv4InterfaceContainer interfaces;
  for (uint32_t i = 0; i < srcNum; i++)
    {
      interfaces = MakeLink ("pfifo", srcs.Get (i), switchs.Get(2 * i), edgeBW, linkDelay, ipv4AddrHelper);
      src_interfaces.Add (interfaces.Get (1));

      MakeLink ("pfifo", switchs.Get (2 * i), routers.Get(2 * i), edgeBW, linkDelay, ipv4AddrHelper);
      MakeLink ("red", routers.Get (2 * i), routers.Get(2 * i + 1), switchBW, linkDelay, ipv4AddrHelper);
      MakeLink ("pfifo", routers.Get (2 * i + 1), switchs.Get(2 * i + 1), edgeBW, linkDelay, ipv4AddrHelper);

      interfaces = MakeLink ("pfifo", switchs.Get(2 * i + 1), dsts.Get(i), edgeBW, linkDelay, ipv4AddrHelper);
      dst_interfaces.Add (interfaces.Get (1));
    }

  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
TxTrace (uint32_t flowId, dcn::C3Tag c3Tag, Ptr<const Packet> p)
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

  if (rx_size.find (fid) == rx_size.end())
    rx_size[fid] = packet->GetSize ();
  else
    rx_size[fid] += packet->GetSize ();

  if(fid >= ds_flowid_base && fid < cs_flowid_base)
    rx_node[0] += packet->GetSize ();
  else
    rx_node[1] += packet->GetSize ();
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

// CS App
void
InstallCsClient (Ptr<Node> node, const Address &sinkAddr, uint32_t tenantId,
                 uint32_t flowId, uint64_t flowSize, uint64_t packetSize,
                 const Time &startTime, const Time &stopTime)
{
  BulkSendHelper clientHelper ("ns3::L2dctSocketFactory", sinkAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  dcn::C3Tag c3Tag;
  c3Tag.SetTenantId (tenantId);
  c3Tag.SetType (dcn::C3Type::CS);
  c3Tag.SetFlowSize (flowSize);
  clientApp.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId, c3Tag));
  // no need to set flow size in socket
}

// DS App
void
DsSocketCreateTrace (uint64_t flowSize, Time deadline, Ptr<Socket> socket)
{
  socket->SetAttribute ("TotalBytes", UintegerValue (flowSize));
  socket->SetAttribute ("Deadline", TimeValue (deadline));
}

void
InstallDsClient (Ptr<Node> node, const Address &sinkAddr, uint32_t tenantId,
                 uint32_t flowId, uint64_t flowSize, uint64_t packetSize, const Time &deadline,
                 const Time &startTime, const Time &stopTime)
{
  BulkSendHelper clientHelper ("ns3::D2tcpSocketFactory", sinkAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  dcn::C3Tag c3Tag;
  c3Tag.SetTenantId (tenantId);
  c3Tag.SetType (dcn::C3Type::DS);
  c3Tag.SetFlowSize (flowSize);
  c3Tag.SetDeadline (startTime + deadline);
  clientApp.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId, c3Tag));
  clientApp.Get (0)->TraceConnectWithoutContext ("SocketCreate", MakeBoundCallback (&DsSocketCreateTrace, flowSize, deadline));
}

Ptr<RandomVariableStream>
GetDataMiningStream (void)
{
  Ptr<EmpiricalRandomVariable> stream = CreateObject<EmpiricalRandomVariable> ();
  stream->SetStream (random_seed);
  stream->CDF (1, 0.0);
  stream->CDF (1, 0.5);
  stream->CDF (2, 0.6);
  stream->CDF (3, 0.7);
  stream->CDF (7, 0.8);
  stream->CDF (267, 0.9);
  stream->CDF (2107, 0.95);
  stream->CDF (66667, 0.99);
  stream->CDF (666667, 1.0);
  min_flow_size = 1;
  max_flow_size = 666667;
  av_flow_size = 5116;
  return stream;
}

Ptr<RandomVariableStream>
GetHadoopStream (void)
{
  Ptr<EmpiricalRandomVariable> stream = CreateObject<EmpiricalRandomVariable> ();
  stream->SetStream (random_seed);
  stream->CDF (1, 0.0);
  stream->CDF (1, 0.1);
  stream->CDF (333, 0.2);
  stream->CDF (666, 0.3);
  stream->CDF (1000, 0.4);
  stream->CDF (3000, 0.6);
  stream->CDF (5000, 0.8);
  stream->CDF (1000000, 0.95);
  stream->CDF (6666667, 1.0);
  min_flow_size = 1;
  max_flow_size = 6666667;
  av_flow_size = 268392;
  return stream;
}

Ptr<RandomVariableStream>
GetWebSearchStream (void)
{
  Ptr<EmpiricalRandomVariable> stream = CreateObject<EmpiricalRandomVariable> ();
  stream->SetStream (random_seed);
  stream->CDF (6, 0.0);
  stream->CDF (6, 0.15);
  stream->CDF (13, 0.2);
  stream->CDF (19, 0.3);
  stream->CDF (33, 0.4);
  stream->CDF (53, 0.53);
  stream->CDF (133, 0.6);
  stream->CDF (667, 0.7);
  stream->CDF (1333, 0.8);
  stream->CDF (3333, 0.9);
  stream->CDF (6667, 0.97);
  stream->CDF (20000, 1.0);
  min_flow_size = 6;
  max_flow_size = 20000;
  av_flow_size = 1138;
  return stream;
}

int
main (int argc, char *argv[])
{
  bool c3pEnable = false;
  bool dsEnable = false;
  bool csEnable = false;
  bool writeResult = true;
  bool writeFlowInfo = true;
  bool writeThroughput = true;
  std::string pathOut ("."); // Current directory

  CommandLine cmd;
  cmd.AddValue ("enableCS", "<0/1> enable CS test", csEnable);
  cmd.AddValue ("enableDS", "<0/1> enable DS test", dsEnable);
  cmd.AddValue ("packetSize", "Size for every packet", packet_size);
  cmd.AddValue ("queueSize", "Queue length for RED queue", queue_size);
  cmd.AddValue ("threhold", "Threhold for RED queue", threhold);
  cmd.AddValue ("workload", "workload type", workload);
  cmd.AddValue ("miceLoad", "network load factor", mice_load);
  cmd.AddValue ("simTime", "simulation time", sim_time);
  cmd.AddValue ("enableC3P", "<0/1> enable C3 in test", c3pEnable);
  cmd.AddValue ("pathOut", "Path to save results", pathOut);
  cmd.AddValue ("writeResult", "<0/1> to write result", writeResult);
  cmd.AddValue ("writeFlowInfo", "<0/1> to write flow info", writeFlowInfo);
  cmd.AddValue ("writeThroughput", "<0/1> to write throughput", writeThroughput);
  cmd.Parse (argc, argv);

  Time globalStartTime = Seconds (0);
  Time globalStopTime = globalStartTime + Seconds (sim_time);
  Time sinkStartTime = globalStartTime;
  Time sinkStopTime = globalStopTime + Seconds (3);
  Time clientStopTime = globalStopTime;

  SetupConfig ();
  SetupTopo (src_num, dst_num, edge_bw, btnk_bw, link_delay);

  // install dctcp socket factory to hosts
  DctcpSocketFactoryHelper dctcpHelper;
  dctcpHelper.AddSocketFactory ("ns3::DctcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::D2tcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::L2dctSocketFactory");
  dctcpHelper.Install (srcs);
  dctcpHelper.Install (dsts);

  if (c3pEnable)
    {
      dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::C3L3_5Protocol");
      l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
      l3_5Helper.Install(srcs);
      l3_5Helper.Install(dsts);

      dcn::C3Division::AddDivisionType (dcn::C3Type::CS, "ns3::dcn::C3CsDivision");
      dcn::C3Division::AddDivisionType (dcn::C3Type::DS, "ns3::dcn::C3DsDivision");
      if (csEnable)
        {
          Ptr<dcn::C3Division> division = dcn::C3Division::CreateDivision (0, dcn::C3Type::CS);
          division->SetAttribute ("Weight", DoubleValue (1.0));
        }
      if (dsEnable)
        {
          Ptr<dcn::C3Division> division = dcn::C3Division::CreateDivision (0, dcn::C3Type::DS);
          division->SetAttribute ("Weight", DoubleValue (1.0));
        }
    }
  // install ip sink on all hosts
  InstallSink (dsts, sink_port, sinkStartTime, sinkStopTime);

  // setup clients
  Ptr<RandomVariableStream> flowSizeStream;
  switch (workload) {
    case 0:
      flowSizeStream = GetDataMiningStream ();
      break;
    case 1:
      flowSizeStream = GetWebSearchStream ();
      break;
    case 2:
      flowSizeStream = GetHadoopStream ();
      break;
    default:
      break;
    }

  // 根据瓶颈链路负载比例以及平均流大小计算出对应的平均流到达时间间隔
  Time avInterArrival = Seconds ((av_flow_size * 1000 * 8)/ (mice_load * btnk_bw.GetBitRate () * srcs.GetN ()));
  auto interArrivalStream = CreateObject<ExponentialRandomVariable> ();
  interArrivalStream->SetStream (random_seed);
  interArrivalStream->SetAttribute ("Mean", DoubleValue (avInterArrival.GetSeconds ()));

  auto hostStream = CreateObject<UniformRandomVariable> ();
  hostStream->SetStream (2);
  hostStream->SetAttribute ("Min", DoubleValue (0));
  hostStream->SetAttribute ("Max", DoubleValue (srcs.GetN () - 1));

  std::stringstream ss;
  // ss << pathOut << "/flow-info.txt";
  ss << pathOut << "/flow-info-" << mice_load << ".txt";
  std::ofstream out (ss.str ());
  if (true)
    {
      uint32_t dsFlowId = ds_flowid_base; 
      for (Time clientStartTime = globalStartTime + Seconds (0.3);
           clientStartTime < clientStopTime;
           clientStartTime += Seconds (interArrivalStream->GetValue ()))
        {
          uint32_t srcIndex = 0; //hostStream->GetInteger ();
          uint32_t dstIndex = 0; //hostStream->GetInteger ();
          //for (;srcIndex == dstIndex; dstIndex = hostStream->GetInteger ());
          uint32_t flowId = dsFlowId++;
          uint32_t flowSize = flowSizeStream->GetInteger () * 1000;
          Time deadline = Seconds (5.0 / 1000 + flowSize / 1e7);
          InstallDsClient (srcs.Get (srcIndex), InetSocketAddress (dst_interfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, flowSize, packet_size, deadline, clientStartTime, clientStopTime);
          if (writeFlowInfo)
            {
              // output flow status
              out << flowId << ", " << flowSize << ", " << clientStartTime.GetSeconds () << ", " << deadline.GetSeconds () << std::endl;
            }
        }
    }

  if (true)
    {
      uint32_t csFlowId = cs_flowid_base;
      for (Time clientStartTime = globalStartTime + Seconds (0.3);
           clientStartTime < clientStopTime;
           clientStartTime += Seconds (interArrivalStream->GetValue ()))
        {
          uint32_t srcIndex = 1;//hostStream->GetInteger ();
          uint32_t dstIndex = 1;//hostStream->GetInteger ();
          //for (;srcIndex == dstIndex; dstIndex = hostStream->GetInteger ());
          uint32_t flowId = csFlowId++;
          uint32_t flowSize = flowSizeStream->GetInteger () * 1000;
          Time deadline = Seconds (5.0 / 1000 + flowSize / 1e7);
          InstallCsClient (srcs.Get (srcIndex), InetSocketAddress (dst_interfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, flowSize, packet_size, clientStartTime, clientStopTime);
          if (writeFlowInfo)
            {
              // output flow status
              out << flowId << ", " << flowSize << ", " << clientStartTime.GetSeconds () << ", " << deadline.GetSeconds () << std::endl;
            }
        }
    }
  out.close ();

  if (writeThroughput)
    {
      Simulator::Schedule (Seconds (sinkStartTime), &CalculateThroughput);
    }

  Simulator::Stop (sinkStopTime);
  Simulator::Run ();

  if (writeResult)
    {
      std::stringstream ss;
      ss << pathOut << "/flow-result-" << mice_load << ".txt";
      std::ofstream out (ss.str ());
      for (auto& entry : tx_time)
        {
          out << entry.first
              << "," << entry.second
              << "," << rx_time[entry.first]
              << "," << rx_size[entry.first]
              << std::endl;
        }
      out.close ();
    }

/*
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
*/
  if (writeThroughput)
    {
      for (auto& resultList : goodput_result)
        {
          std::stringstream ss;
          ss << pathOut << "/throughput-" << mice_load << "-" <<resultList.first << ".txt";
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
