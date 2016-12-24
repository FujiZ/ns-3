#include "ns3/log.h"

#include "c3-division.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Division");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Division);

TypeId
C3Division::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Division")
      .SetParent<RateController> ()
      .SetGroupName ("DCN")
      .AddConstructor<C3Division> ()
  ;
  return tid;
}

C3Division::C3Division (Ipv4Address src, Ipv4Address dst):
  m_source (src), m_destination (dst)
{
  NS_LOG_FUNCTION (this);
}

C3Division::~C3Division ()
{
  NS_LOG_FUNCTION (this);
}

void C3Division::SetRoute (Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << route);
  this->m_route = route;
}

void C3Division::SetForwardTargetCallback (ForwardTargetCallback cb)
{
  NS_LOG_FUNCTION (this << cb);
  this->m_forwardTargetCallback = cb;
}

void C3Division::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ///\todo dispose all tunnels inside class
  m_forwardTargetCallback.Nullify ();
  m_route = 0;
  RateController::DoDispose ();
}

void C3Division::Forward (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  m_forwardTargetCallback (p, m_source, m_destination, m_route);
}

} //namespace dcn
} //namespace ns3
