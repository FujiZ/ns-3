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

NS_LOG_COMPONENT_DEFINE ("C3pTest");

// topo attributes
uint32_t cores_num = 1;
uint32_t aggs_num = 2;
uint32_t tors_num = 2;
uint32_t hosts_num = 10;
Time link_delay ("0.025ms");
DataRate btnk_bw ("1Gbps");
DataRate non_btnk_bw ("10Gbps");
uint32_t queue_size = 250;
uint32_t threhold = 65;

// c3 attributes
Time tunnel_interval ("1ms");
Time division_interval ("3ms");

// time attributes
double sim_time = 15;

// attributes
double mice_load = 0.4;
uint32_t packet_size = 1000;
uint32_t min_flow_size = 1;
uint32_t max_flow_size = 666667;
uint32_t av_flow_size = 5116;
uint16_t sink_port = 50000;
uint64_t random_seed = 2;

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
  Config::SetDefault ("ns3::dcn::TokenBucketFilter::DataRate", DataRateValue (btnk_bw));
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
  Config::SetDefault ("ns3::dcn::C3Tunnel::Interval", TimeValue (tunnel_interval));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Gamma", DoubleValue (1.0 / 16));
  Config::SetDefault ("ns3::dcn::C3Tunnel::Rate", DataRateValue (DataRate ("250Mbps")));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MaxRate", DataRateValue (btnk_bw));
  Config::SetDefault ("ns3::dcn::C3Tunnel::MinRate", DataRateValue (DataRate ("1Mbps")));
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
SetupTopo (uint32_t coresNum, uint32_t aggsNum, uint32_t torsNum, uint32_t hostsNum,
           DataRate coreAggBW, DataRate aggTorBW, DataRate torHostBW, Time linkDelay)
{
  InternetStackHelper internetHelper;
  Ipv4AddressHelper  ipv4AddrHelper ("10.1.1.0", "255.255.255.0");

  NodeContainer cores;
  cores.Create (coresNum);
  internetHelper.Install (cores);
  for (NodeContainer::Iterator i = cores.Begin (); i != cores.End (); ++i)
    {
      NodeContainer aggs;
      aggs.Create (aggsNum);
      internetHelper.Install (aggs);
      for (NodeContainer::Iterator j = aggs.Begin (); j != aggs.End (); ++j)
        {
          MakeLink ("red", *i, *j, coreAggBW, linkDelay, ipv4AddrHelper);
          NodeContainer tors;
          tors.Create (torsNum);
          internetHelper.Install (tors);
          for (NodeContainer::Iterator k = tors.Begin (); k != tors.End (); ++k)
            {
              MakeLink ("red", *j, *k, aggTorBW, linkDelay, ipv4AddrHelper);
              NodeContainer endhosts;
              endhosts.Create (hostsNum);
              internetHelper.Install (endhosts);
              for (NodeContainer::Iterator l = endhosts.Begin (); l != endhosts.End (); ++l)
                {
                  auto interfaces = MakeLink ("pfifo", *k, *l, torHostBW, linkDelay, ipv4AddrHelper);
                  hosts_interfaces.Add (interfaces.Get (1));
                }
              hosts.Add (endhosts);
            }
        }
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
  stream->SetStream (2);
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

int
main (int argc, char *argv[])
{
  bool c3pEnable = false;
  bool dsEnable = false;
  bool csEnable = false;
  bool writeResult = false;
  bool writeFlowInfo = false;
  std::string pathOut ("."); // Current directory

  CommandLine cmd;
  cmd.AddValue ("enableCS", "<0/1> enable CS test", csEnable);
  cmd.AddValue ("enableDS", "<0/1> enable DS test", dsEnable);
  cmd.AddValue ("packetSize", "Size for every packet", packet_size);
  cmd.AddValue ("queueSize", "Queue length for RED queue", queue_size);
  cmd.AddValue ("threhold", "Threhold for RED queue", threhold);
  cmd.AddValue ("miceLoad", "network load factor", mice_load);
  cmd.AddValue ("simTime", "simulation time", sim_time);
  cmd.AddValue ("enableC3P", "<0/1> enable C3 in test", c3pEnable);
  cmd.AddValue ("pathOut", "Path to save results", pathOut);
  cmd.AddValue ("writeResult", "<0/1> to write result", writeResult);
  cmd.AddValue ("writeFlowInfo", "<0/1> to write flow info", writeFlowInfo);
  cmd.Parse (argc, argv);

  Time globalStartTime = Seconds (0);
  Time globalStopTime = globalStartTime + Seconds (sim_time);
  Time sinkStartTime = globalStartTime;
  Time sinkStopTime = globalStopTime + Seconds (3);
  Time clientStopTime = globalStopTime;

  SetupConfig ();
  SetupTopo (cores_num, aggs_num, tors_num, hosts_num,
             non_btnk_bw, non_btnk_bw, btnk_bw, link_delay);

  // install dctcp socket factory to hosts
  DctcpSocketFactoryHelper dctcpHelper;
  dctcpHelper.AddSocketFactory ("ns3::DctcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::D2tcpSocketFactory");
  dctcpHelper.AddSocketFactory ("ns3::L2dctSocketFactory");
  dctcpHelper.Install (hosts);

  if (c3pEnable)
    {
      dcn::IpL3_5ProtocolHelper l3_5Helper ("ns3::dcn::C3L3_5Protocol");
      l3_5Helper.AddIpL4Protocol ("ns3::TcpL4Protocol");
      l3_5Helper.Install(hosts);

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
  InstallSink (hosts, sink_port, sinkStartTime, sinkStopTime);

  // setup clients
  Ptr<RandomVariableStream> flowSizeStream = GetDataMiningStream ();

  Time avInterArrival = Seconds ((av_flow_size * 1000 * 8)/ (mice_load * btnk_bw.GetBitRate () * hosts.GetN ()));
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
      uint32_t dsFlowId = 10000;
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
      uint32_t csFlowId = 20000;
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
