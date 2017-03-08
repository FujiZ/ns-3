#include "c3-division.h"

#include "ns3/log.h"

#include "c3-tag.h"
#include "c3-header.h"

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
  ;
  return tid;
}

C3Division::C3Division (const Ipv4Address& src, const Ipv4Address& dst)
  : m_source (src),
    m_destination (dst)
{
  NS_LOG_FUNCTION (this);
  m_tunnel = CreateObject<C3DsTunnel> ();
  m_tunnel->SetForwardTarget (MakeCallback (&C3Division::ForwardDown, this));
}

C3Division::~C3Division ()
{
  NS_LOG_FUNCTION (this);
}

void
C3Division::SetRoute (Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this);
  this->m_route = route;
}

void
C3Division::SetDownTarget (DownTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  this->m_downTarget = cb;
}

void
C3Division::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  C3Tag c3Tag;
  NS_ASSERT (packet->PeekPacketTag (c3Tag));
  m_tunnel->Send (packet);
}

void
C3Division::SetUpTarget (UpTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  this->m_upTarget = cb;
}

enum IpL4Protocol::RxStatus
C3Division::Receive (Ptr<Packet> packet,
                     Ipv4Header const &header,
                     Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header);
  C3Header c3Header;
  packet->PeekHeader (c3Header);
  if (c3Header.GetFlags () & C3Header::ACK)
    {
      ++m_totalAck;
      if (c3Header.GetFlags () & C3Header::ECE)
        {
          ++m_markedAck;
        }
    }
  return m_upTarget (packet, header, incomingInterface);
}

uint64_t
C3Division::UpdateRateRequest (void)
{
  NS_LOG_FUNCTION (this);
  m_rateRequest = m_tunnel->UpdateRateRequest ();
  return m_rateRequest;
}

void
C3Division::SetRateResponse (uint64_t rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rateResponse = rate;
  m_tunnel->SetRateResponse (rate);
}

void
C3Division::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ///\todo dispose all tunnels inside class
  m_tunnel = 0;
  m_upTarget.Nullify ();
  m_downTarget.Nullify ();
  m_route = 0;
  RateController::DoDispose ();
}

void
C3Division::ForwardDown (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  m_downTarget (packet, m_source, m_destination, m_route);
}

} //namespace dcn
} //namespace ns3
