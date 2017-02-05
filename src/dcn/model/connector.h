#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief implement the ns-2 Connector in ns-3
 * the base class of Connector in ns-2 with only a single neighbor
 */
class Connector : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual ~Connector ();

public:
  /**
   * \brief callback to send packets
   */
  typedef Callback<void, Ptr<Packet> > SendTargetCallback;
  /**
   * \brief callback to drop packets
   */
  typedef Callback<void, Ptr<const Packet> > DropTargetCallback;
  /**
   * This method allows a caller to set the current send target callback
   *
   * \param cb current Callback for send target
   */
  void SetSendTarget (SendTargetCallback cb);
  /**
   * This method allows a caller to get the current send target callback
   *
   * \return current Callback for send target
   */
  SendTargetCallback GetSendTarget (void) const;
  /**
   * This method allows a caller to set the current drop target callback
   *
   * \param cb current Callback for drop target
   */
  void SetDropTarget (DropTargetCallback cb);
  /**
   * This method allows a caller to get the current drop target callback
   *
   * \return current Callback for drop target
   */
  DropTargetCallback GetDropTarget (void) const;
  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for filter to receive
   */
  virtual void Send (Ptr<Packet> p) = 0;

protected:
  virtual void DoDispose (void);

protected:
  SendTargetCallback m_sendTarget;
  DropTargetCallback m_dropTarget;
};

} //namespace dcn
} //namespace ns3

#endif // CONNECTOR_H
