#include "c3-division.h"

#include "cmath"

#include "ns3/log.h"
#include "ns3/object-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3Division");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (C3Division);

C3Division::DivisionList_t C3Division::m_divisionList;
C3Division::DivisionTypeList_t C3Division::m_divisionTypeList;
Time C3Division::m_startTime;
Time C3Division::m_stopTime;
Time C3Division::m_interval;
Timer C3Division::m_timer (Timer::CANCEL_ON_DESTROY);

TypeId
C3Division::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::C3Division")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddAttribute ("TenantId",
                     "The tenantId of division",
                     UintegerValue (0),
                     MakeUintegerAccessor (&C3Division::m_tenantId),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("Weight",
                     "The weight for Division",
                     DoubleValue (1.0),
                     MakeDoubleAccessor (&C3Division::m_weight),
                     MakeDoubleChecker<double> (0.0))
  ;
  return tid;
}

C3Division::C3Division (C3Type type)
  : m_tenantId (0),
    m_type (type),
    m_weight (0.0)
{
  NS_LOG_FUNCTION (this);
}

C3Division::~C3Division ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<C3Division>
C3Division::GetDivision (uint32_t tenantId, C3Type type)
{
  NS_LOG_FUNCTION (tenantId << static_cast<uint8_t> (type));

  auto iter = m_divisionList.find (std::make_pair (tenantId, type));
  if (iter != m_divisionList.end ())
    {
      return iter->second;
    }
  else
    {
      return 0;
    }
}

Ptr<C3Division>
C3Division::CreateDivision (uint32_t tenantId, C3Type type)
{
  NS_LOG_FUNCTION (tenantId << static_cast<uint8_t> (type));

  auto iter = m_divisionList.find (std::make_pair (tenantId, type));
  if (iter != m_divisionList.end ())
    {
      return iter->second;
    }
  else
    {
      ObjectFactory factory;
      factory.SetTypeId (m_divisionTypeList[type]);
      factory.Set ("TenantId", UintegerValue (tenantId));
      Ptr<C3Division> division = factory.Create<C3Division> ();
      m_divisionList[std::make_pair(tenantId, type)] = division;
      return division;
    }
}

void
C3Division::AddDivisionType (C3Type type, std::string tid)
{
  NS_LOG_FUNCTION (tid);
  m_divisionTypeList[type] = tid;
}

void
C3Division::Start (Time start)
{
  NS_LOG_FUNCTION (start);
  m_startTime = start;
  if (!m_timer.IsRunning ())
    {
      NS_LOG_DEBUG ("change start time");
      m_timer.SetFunction (&C3Division::UpdateAll);
      // reschedule start event
      m_timer.Remove ();
      m_timer.Schedule (m_startTime);
    }
}

void
C3Division::Stop (Time stop)
{
  NS_LOG_FUNCTION (stop);
  m_stopTime = stop;
}

void
C3Division::SetInterval (Time interval)
{
  NS_LOG_FUNCTION (interval);
  m_interval = interval;
}

void
C3Division::UpdateAll (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (auto it = m_divisionList.begin (); it != m_divisionList.end (); ++it)
    {
      Ptr<C3Division> division = it->second;
      division->Update ();
    }
  if (Simulator::Now () + m_interval <= m_stopTime)
    {
      m_timer.Schedule (m_interval);
    }
}

void
C3Division::Update (void)
{
  NS_LOG_FUNCTION (this);
  double weight = 0.0;
  for (auto it = m_tunnelList.begin (); it != m_tunnelList.end (); ++it)
    {
      Ptr<C3Tunnel> tunnel = it->second;
      tunnel->UpdateInfo ();
      weight += tunnel->GetWeightRequest ();
    }
  // here to check if weight == 0.0
  double lambda = std::fabs (weight) > 10e-7 ? m_weight / weight : 0.0;    // lambda: scale factor
  for (auto it = m_tunnelList.begin (); it != m_tunnelList.end (); ++it)
    {
      Ptr<C3Tunnel> tunnel = it->second;
      tunnel->SetWeight (lambda * tunnel->GetWeightRequest ());
      tunnel->UpdateRate ();
      tunnel->Schedule ();
    }
}

void
C3Division::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_tunnelList.clear ();
  Object::DoDispose ();
}

uint32_t
C3Division::GetTenantId (void)
{
  return m_tenantId;
}

} //namespace dcn
} //namespace ns3
