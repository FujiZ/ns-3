#include "c3-tunnel.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Tunnel");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Tunnel);

TypeId
C3Tunnel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Tunnel")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddAttribute ("G",
                     "0 < G < 1 is the weight given to new samples"
                     "against the past in the estimation of alpha.",
                     DoubleValue (0.625),
                     MakeDoubleAccessor (&C3Tunnel::m_g),
                     MakeDoubleChecker<double> (0.0, 1.0))
      .AddTraceSource ("Alpha",
                       "an estimate of the fraction of packets that are marked",
                       MakeTraceSourceAccessor (&C3Tunnel::m_alpha),
                       "ns3::TracedValueCallback::Double")
      .AddTraceSource ("Weight",
                       "Weight alloced to the tunnel.",
                       MakeTraceSourceAccessor (&C3Tunnel::m_weight),
                       "ns3::TracedValueCallback::Double")
      .AddTraceSource ("WeightRequest",
                       "Weight required by the tunnel.",
                       MakeTraceSourceAccessor (&C3Tunnel::m_weightRequest),
                       "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

C3Tunnel::C3Tunnel (uint32_t tenantId, C3Type type,
                    const Ipv4Address &src, const Ipv4Address &dst)
  : m_src (src),
    m_dst (dst),
    m_alpha (0.0),
    m_g (0.625),
    m_weight (0.0),
    m_weightRequest (0.0)
{
  NS_LOG_FUNCTION (this);
  m_ecnRecorder = C3EcnRecorder::CreateEcnRecorder (tenantId, type, src, dst);
}

C3Tunnel::~C3Tunnel ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Tunnel::SetRoute (Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << route);
  m_route = route;
}

void
C3Tunnel::SetForwardTarget (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardTarget = cb;
}

void
C3Tunnel::UpdateInfo (void)
{
  NS_LOG_FUNCTION (this);

  UpdateAlpha ();
  double weightRequest = 0;
  for (auto it = m_flowList.begin (); it != m_flowList.end (); ++it)
    {
      Ptr<C3Flow> flow = it->second;
      flow->UpdateInfo ();
      weightRequest += flow->GetWeight ();
    }
  m_weightRequest = weightRequest;
}

double
C3Tunnel::GetWeightRequest (void) const
{
  return m_weightRequest;
}

void
C3Tunnel::SetWeight (double weight)
{
  NS_LOG_FUNCTION (this << weight);
  m_weight = weight;
}

void
C3Tunnel::UpdateRate (void)
{
  NS_LOG_FUNCTION (this);
  if (m_ecnRecorder->GetMarkedCount ())
    {
      NS_LOG_DEBUG ("Congestion detected");
      m_rate = DataRate ((1 - m_alpha * m_weight) * m_rate.GetBitRate ());
    }
  else
    {
      NS_LOG_DEBUG ("No congestion");
      m_rate = DataRate ((1 + m_weight) * m_rate.GetBitRate ());
    }
  m_ecnRecorder->Reset ();
}

void
C3Tunnel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ///\todo dispose ecn recorder
  m_ecnRecorder = 0;
  m_forwardTarget.Nullify ();
  m_route = 0;
  m_flowList.clear ();
  Object::DoDispose ();
}

void
C3Tunnel::Forward (Ptr<Packet> packet,  uint8_t protocol)
{
  NS_LOG_FUNCTION (this << packet << (uint32_t)protocol);
  m_forwardTarget (packet, m_src, m_dst, protocol, m_route);
}

DataRate
C3Tunnel::GetRate (void) const
{
  return m_rate;
}

void
C3Tunnel::UpdateAlpha (void)
{
  NS_LOG_FUNCTION (this);
  m_alpha = (1 - m_g) * m_alpha + m_g * m_ecnRecorder->GetRatio ();
}

} //namespace dcn
} //namespace ns3
