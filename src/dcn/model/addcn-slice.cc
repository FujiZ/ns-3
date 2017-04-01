#include "addcn-slice.h"

#include "cmath"

#include "ns3/log.h"
#include "ns3/object-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ADDCNSlice");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (ADDCNSlice);

ADDCNSlice::TupleSliceList_t ADDCNSlice::m_tupleSliceList;
ADDCNSlice::SliceList_t ADDCNSlice::m_sliceList;
ADDCNSlice::SliceTypeList_t ADDCNSlice::m_sliceTypeList;
Time ADDCNSlice::m_startTime;
Time ADDCNSlice::m_stopTime;
Time ADDCNSlice::m_interval;
Timer ADDCNSlice::m_timer (Timer::CANCEL_ON_DESTROY);

TypeId
ADDCNSlice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::ADDCNSlice")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
      .AddAttribute ("TenantId",
                     "The tenantId of slice",
                     UintegerValue (0),
                     MakeUintegerAccessor (&ADDCNSlice::m_tenantId),
                     MakeUintegerChecker<uint32_t> ())
      /*
      .AddAttribute ("C3Type",
                     "The type of slice, LS DS CS NONE",
                     EnumValue (C3Type::NONE),
                     MakeEnumAccessor (&ADDCNSlice::GetC3Type, &ADDCNSlice::SetC3Type),
                     MakeEnumChecker (C3Type::NONE, "None",
                                      C3Type::LS, "LS",
                                      C3Type::DS, "DS",
                                      C3Type::CS, "CS"))
      */
      .AddAttribute ("Weight",
                     "The weight for Slice",
                     DoubleValue (1.0),
                     MakeDoubleAccessor (&ADDCNSlice::m_weight),
                     MakeDoubleChecker<double> (0.0))
      .AddConstructor<ADDCNSlice> ();
  ;
  return tid;
}

ADDCNSlice::ADDCNSlice ()
  : m_tenantId (0),
    m_type (C3Type::NONE),
    m_weight (0.0),
    m_scale (1.0)
{
  NS_LOG_FUNCTION (this);
}

ADDCNSlice::~ADDCNSlice ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<ADDCNSlice>
ADDCNSlice::GetSliceFromTuple (const ADDCNFlow::FiveTuple &tuple)
{
  NS_LOG_FUNCTION ("GetSliceFormTuple ");
  auto iter = m_tupleSliceList.find (tuple);
  NS_ASSERT(iter != m_tupleSliceList.end ());
  return iter->second;
}
Ptr<ADDCNSlice>
ADDCNSlice::GetSlice (uint32_t tenantId, C3Type type)
{
  NS_LOG_FUNCTION ("GetSlice " << tenantId << static_cast<uint8_t> (type));

  auto iter = m_sliceList.find (std::make_pair (tenantId, type));
  if (iter != m_sliceList.end ())
    {
      return iter->second;
    }
  else
    {
      return CreateSlice(tenantId, type);
    }
}

Ptr<ADDCNSlice>
ADDCNSlice::CreateSlice (uint32_t tenantId, C3Type type)
{
  NS_LOG_FUNCTION (tenantId << static_cast<uint8_t> (type));

  auto iter = m_sliceList.find (std::make_pair (tenantId, type));
  if (iter != m_sliceList.end ())
    {
      return iter->second;
    }
  else
    {
      ObjectFactory factory;
      factory.SetTypeId ("ns3::dcn::ADDCNSlice");
      factory.Set ("TenantId", UintegerValue (tenantId));
      Ptr<ADDCNSlice> slice = factory.Create<ADDCNSlice> ();
      slice->SetSliceType(type);
      //slice->SetTenantId(tenantId);
      m_sliceList[std::make_pair(tenantId, type)] = slice;
      return slice;
    }
}

Ptr<ADDCNFlow>
ADDCNSlice::GetFlow(const ADDCNFlow::FiveTuple &tup)
{
  NS_LOG_FUNCTION (tup.sourceAddress << tup.destinationAddress << tup.sourcePort << tup.destinationPort << tup.protocol);

  auto it = m_flowList.find (tup);
  if (it != m_flowList.end ())
  {
    return it->second;
  }
  else
  {
    Ptr<ADDCNFlow> flow = CreateObject<ADDCNFlow> ();
    flow->SetFiveTuple(tup);
    flow->UpdateScale(m_scale);
    m_flowList[tup] = flow;
    m_tupleSliceList[tup] = this;
    return flow;
  }
}

void
ADDCNSlice::SetSliceType (C3Type type)
{
  NS_LOG_FUNCTION (this);
  m_type = type;
}

C3Type
ADDCNSlice::GetSliceType ()
{
  NS_LOG_FUNCTION (this);
  return m_type;
}

void
ADDCNSlice::AddSliceType (C3Type type, std::string tid)
{
  NS_LOG_FUNCTION (tid);
  m_sliceTypeList[type] = tid;
}

void
ADDCNSlice::Start (Time start)
{
  NS_LOG_FUNCTION (start);
  m_startTime = start;
  if (!m_timer.IsRunning ())
    {
      NS_LOG_DEBUG ("change start time");
      m_timer.SetFunction (&ADDCNSlice::UpdateAll);
      // reschedule start event
      m_timer.Remove ();
      m_timer.Schedule (m_startTime);
    }
}

void
ADDCNSlice::Stop (Time stop)
{
  NS_LOG_FUNCTION (stop);
  m_stopTime = stop;
}

void
ADDCNSlice::SetInterval (Time interval)
{
  NS_LOG_FUNCTION (interval);
  m_interval = interval;
}

void
ADDCNSlice::UpdateAll (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (auto it = m_sliceList.begin (); it != m_sliceList.end (); ++it)
    {
      Ptr<ADDCNSlice> slice = it->second;
      slice->Update ();
    }
  if (Simulator::Now () + m_interval <= m_stopTime)
    {
      m_timer.Schedule (m_interval);
    }
}

void
ADDCNSlice::Update (void)
{
  NS_LOG_FUNCTION (this);
  double weight = 0.0;
  // TODO Change to update flow weight in that slice
  for(auto it = m_flowList.begin(); it != m_flowList.end(); it++)
  {
     Ptr<ADDCNFlow> flow = it->second;
     weight += flow->GetWeight();
  }
  m_scale = std::fabs(weight) > 10e-7 ? m_weight / weight : 0.0;

  for(auto it = m_flowList.begin(); it != m_flowList.end(); it++)
  {
     Ptr<ADDCNFlow> flow = it->second;
     flow->UpdateScale(m_scale);
  }
  /*
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
   */
}

void
ADDCNSlice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_flowList.clear ();
  Object::DoDispose ();
}

uint32_t
ADDCNSlice::GetTenantId (void)
{
  return m_tenantId;
}

void
ADDCNSlice::SetTenantId(uint32_t tid)
{
  m_tenantId = tid;
}
} //namespace dcn
} //namespace ns3
