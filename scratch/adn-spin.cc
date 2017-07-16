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

NS_LOG_COMPONENT_DEFINE ("ADN_SPINE");

// topo attributes
uint32_t spin_num = 4;
uint32_t leaf_num = 4;
uint32_t host_num = 4;
Time link_delay ("0.0375ms");
DataRate btnk_bw ("1Gbps");
DataRate non_btnk_bw ("10Gbps");
uint32_t queue_size = 250;
uint32_t threhold = 65;

// c3 attributes
Time tunnel_interval ("1ms");
Time division_interval ("3ms");
DataRate tunnel_bw ("500Mbps"); //!< initial tunnel bw

// time attributes
double sim_time = 15;

// attributes
uint32_t workload = 0;
double mice_load = 0.4;
uint32_t packet_size = 1000;
uint32_t min_flow_size = 1;
uint32_t max_flow_size = 666667;
uint32_t av_flow_size = 5116;
uint16_t sink_port = 50000;
int64_t random_seed = 2;

NodeContainer hosts;    //!< all end hosts
Ipv4InterfaceContainer hosts_interfaces;    //!< hosts interfaces include address

std::map<uint32_t, double> tx_time;   //!< time the first packet is sent
std::map<uint32_t, double> rx_time;   //!< time the last packet is received
std::map<uint32_t, uint64_t> rx_size;   //!< total rx bytes

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
SetupTopo (uint32_t spinNum, uint32_t leafNum, uint32_t hostNum,
           DataRate spin2leaf, DataRate leaf2host, Time linkDelay)
{
  InternetStackHelper internetHelper;
  Ipv4AddressHelper  ipv4AddrHelper ("10.1.1.0", "255.255.255.0");

  NodeContainer spines, leafs;
  spines.Create (spinNum);
  leafs.Create (leafNum);
  internetHelper.Install (spines);
  internetHelper.Install (leafs);
  for (NodeContainer::Iterator i = spines.Begin (); i != spines.End (); ++i)
    {
      for (NodeContainer::Iterator j = leafs.Begin (); j != leafs.End (); ++j)
        {
          MakeLink ("red", *i, *j, spin2leaf, linkDelay, ipv4AddrHelper);
        }
    }

  for (NodeContainer::Iterator k = leafs.Begin (); k != leafs.End (); ++k)
    {
      NodeContainer endhosts;
      endhosts.Create (hostNum);
      internetHelper.Install (endhosts);
      for (NodeContainer::Iterator l = endhosts.Begin (); l != endhosts.End (); ++l)
        {
          auto interfaces = MakeLink ("pfifo", *k, *l, leaf2host, linkDelay, ipv4AddrHelper);
          hosts_interfaces.Add (interfaces.Get (1));
        }
      hosts.Add (endhosts);
    }
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
TxTrace (uint32_t flowId, /*dcn::C3Tag c3Tag, */Ptr<const Packet> p)
{
  //FlowIdTag flowIdTag;
  //flowIdTag.SetFlowId (flowId);
  //p->AddPacketTag (flowIdTag);
  //p->AddPacketTag (c3Tag);
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
  dcn::AddcnBulkSendHelper clientHelper ("ns3::L2dctSocketFactory", sinkAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  auto it = clientApp.Begin();
  Ptr<dcn::AddcnBulkSendApplication> app = StaticCast<dcn::AddcnBulkSendApplication> (*it);

  app->SetTenantId (tenantId);
  app->SetFlowType (dcn::C3Type::CS);
  app->SetFlowId (flowId);
  app->SetFlowSize (flowSize);
  app->SetSegSize (packetSize);
  app->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId));
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
  dcn::AddcnBulkSendHelper clientHelper ("ns3::D2tcpSocketFactory", sinkAddr);
  clientHelper.SetAttribute ("MaxBytes", UintegerValue (flowSize));
  clientHelper.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer clientApp = clientHelper.Install (node);
  clientApp.Start (startTime);
  clientApp.Stop (stopTime);

  auto it = clientApp.Begin();
  Ptr<dcn::AddcnBulkSendApplication> app = StaticCast<dcn::AddcnBulkSendApplication> (*it);

  app->SetTenantId (tenantId);
  app->SetFlowType (dcn::C3Type::DS);
  app->SetFlowId (flowId);
  app->SetFlowSize (flowSize);
  app->SetSegSize (packetSize);
  app->SetDeadline (startTime + deadline);
  app->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TxTrace, flowId));
  app->TraceConnectWithoutContext ("SocketCreate", MakeBoundCallback (&DsSocketCreateTrace, flowSize, deadline));
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
  bool adnEnable = false;
  bool dsEnable = false;
  bool csEnable = false;
  bool writeResult = false;
  bool writeFlowInfo = false;
  bool useECMP = false;
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
  cmd.AddValue ("enableADN", "<0/1> enable ADN in test", adnEnable);
  cmd.AddValue ("pathOut", "Path to save results", pathOut);
  cmd.AddValue ("writeResult", "<0/1> to write result", writeResult);
  cmd.AddValue ("writeFlowInfo", "<0/1> to write flow info", writeFlowInfo);
  cmd.AddValue ("enableECMP", "<0/1> enable flow-level ecmp", useECMP);
  cmd.Parse (argc, argv);

  Time globalStartTime = Seconds (0);
  Time globalStopTime = globalStartTime + Seconds (sim_time);
  Time sinkStartTime = globalStartTime;
  Time sinkStopTime = globalStopTime + Seconds (3);
  Time clientStopTime = globalStopTime;

  if (useECMP)
    {
      //Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_HASH"));
      Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_RANDOM"));
      //Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue ("ECMP_FLOWCELL"));
    }

  SetupConfig ();
  SetupTopo (spin_num, leaf_num, host_num,
             btnk_bw, non_btnk_bw, link_delay);

  // install dctcp socket factory to hosts
  DctcpSocketFactoryHelper dctcpHelper;
  dctcpHelper.AddSocketFactory ("ns3::DctcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::D2tcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::L2dctSocketFactory");
  dctcpHelper.Install (hosts);

  if (adnEnable)
    {
      dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::ADDCNL3_5Protocol");
      l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
      l3_5Helper.Install(hosts);

      ns3::dcn::ADDCNSlice::SetInterval(MilliSeconds (2));
      ns3::dcn::ADDCNSlice::Start(Seconds(globalStartTime));
      ns3::dcn::ADDCNSlice::Stop(Seconds(globalStopTime));
      /*
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
      */
    }
  // install ip sink on all hosts
  InstallSink (hosts, sink_port, sinkStartTime, sinkStopTime);

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

  Time avInterArrival = Seconds ((av_flow_size * 1000 * 8)/ (mice_load * btnk_bw.GetBitRate () * host_num));
  auto interArrivalStream = CreateObject<ExponentialRandomVariable> ();
  interArrivalStream->SetStream (random_seed);
  interArrivalStream->SetAttribute ("Mean", DoubleValue (avInterArrival.GetSeconds ()));

  auto hostStream = CreateObject<UniformRandomVariable> ();
  hostStream->SetStream (2);
  hostStream->SetAttribute ("Min", DoubleValue (0));
  hostStream->SetAttribute ("Max", DoubleValue (hosts.GetN () - 1));

  std::stringstream ss;
  // ss << pathOut << "/flow-info.txt";
  ss << pathOut << "/flow-info-" << mice_load << ".txt";
  std::ofstream out (ss.str ());
  if (dsEnable)
    {
      uint32_t dsFlowId = 100000;
      for (Time clientStartTime = globalStartTime + Seconds (0.3);
           clientStartTime < clientStopTime;
           clientStartTime += Seconds (interArrivalStream->GetValue ()))
        {
          uint32_t srcIndex = hostStream->GetInteger ();
          uint32_t dstIndex = hostStream->GetInteger ();
          for (;srcIndex == dstIndex; dstIndex = hostStream->GetInteger ());
          uint32_t flowId = dsFlowId++;
          uint32_t flowSize = flowSizeStream->GetInteger () * 1000;
          Time deadline = Seconds (5.0 / 1000 + flowSize / 1e7);
          InstallDsClient (hosts.Get (srcIndex), InetSocketAddress (hosts_interfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, flowSize, packet_size, deadline, clientStartTime, clientStopTime);
          if (writeFlowInfo)
            {
              // output flow status
              out << flowId << ", " << flowSize << ", " << clientStartTime.GetSeconds () << ", " << deadline.GetSeconds () << std::endl;
            }
        }
    }

  if (csEnable)
    {
      uint32_t csFlowId = 200000;
      for (Time clientStartTime = globalStartTime + Seconds (0.3);
           clientStartTime < clientStopTime;
           clientStartTime += Seconds (interArrivalStream->GetValue ()))
        {
          uint32_t srcIndex = hostStream->GetInteger ();
          uint32_t dstIndex = hostStream->GetInteger ();
          for (;srcIndex == dstIndex; dstIndex = hostStream->GetInteger ());
          uint32_t flowId = csFlowId++;
          uint32_t flowSize = flowSizeStream->GetInteger () * 1000;
          Time deadline = Seconds (5.0 / 1000 + flowSize / 1e7);
          InstallCsClient (hosts.Get (srcIndex), InetSocketAddress (hosts_interfaces.GetAddress (dstIndex), sink_port), 0,
                           flowId, flowSize, packet_size, clientStartTime, clientStopTime);
          if (writeFlowInfo)
            {
              // output flow status
              out << flowId << ", " << flowSize << ", " << clientStartTime.GetSeconds () << ", " << deadline.GetSeconds () << std::endl;
            }
        }
    }
  out.close ();

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
  Simulator::Destroy ();
  return 0;
}
