#include "connector.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Connector");

namespace dcn {

NS_OBJECT_ENSURE_REGISTERED (Connector);

TypeId
Connector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::dcn::Connector")
      .SetParent<Object> ()
      .SetGroupName ("DCN")
  ;
  return tid;
}

Connector::~Connector ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Connector::SetSendTarget (SendTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_sendTarget = cb;
}

Connector::SendTargetCallback
Connector::GetSendTarget (void) const
{
  return m_sendTarget;
}

void
Connector::SetDropTarget (DropTargetCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_dropTarget = cb;
}

Connector::DropTargetCallback
Connector::GetDropTarget (void) const
{
  return m_dropTarget;
}

void
Connector::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_dropTarget.Nullify ();
  m_sendTarget.Nullify ();
  Object::DoDispose ();
}

}
}
