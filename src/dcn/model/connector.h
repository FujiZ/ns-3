#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "ns3/object.h"

namespace ns3 {

class Packet;

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
  /**
   * \brief callback to send packets
   */
  typedef Callback<void,Ptr<Packet> > SendTargetCallback;
  /**
   * \brief callback to drop packets
   */
  typedef Callback<void,Ptr<Packet> > DropTargetCallback;
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
  void SetDropTarget (SendTargetCallback cb);
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
  void Receive (Ptr<Packet> p);

protected:
  virtual void DoDispose (void);
  /**
   * \brief send a packet to the sendTarget
   * \param p the packet to send
   */
  void Send (Ptr<Packet> p);
  /**
   * \brief call dropTarget
   * \param p the packet to drop
   */
  void Drop (Ptr<Packet> p);

  /**
   * \brief implementation of receive(p)
   * \param p the packet for filter to receive
   */
  virtual void DoReceive(Ptr<Packet> p) = 0;

private:
  SendTargetCallback m_sendTarget;
  DropTargetCallback m_dropTarget;
};

} //namespace dcn
} //namespace ns3

#endif // CONNECTOR_H
