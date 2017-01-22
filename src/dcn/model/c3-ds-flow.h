#ifndef C3_DS_FLOW_H
#define C3_DS_FLOW_H

#include <stdint.h>

#include "c3-flow.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the class for Deadline Sensitive Flow
 */
class C3DsFlow : public C3Flow
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  C3DsFlow ();
  virtual ~C3DsFlow ();

  //inherited from RateController
  virtual uint64_t UpdateRateRequest (void);
  virtual void SetRateResponse (uint64_t rate);
  //inherited from C3Flow
  virtual void Send (Ptr<Packet> p);
private:
  /**
   * \brief NotifyPacketSend
   * \param p the packet to send
   * called when tbf send a packet out
   */
  void NotifyPacketSend (Ptr<Packet> p);
private:
  int32_t m_flowSize;    //!< the total size of current flow
  int32_t m_sendedSize;  //!< the sended size of current flow
  int32_t m_bufferSize;  //!< the size of current buffer
  Time m_deadline;        //!< deadline of current flow
};

} //namespace dcn
} //namespace ns3

#endif // C3_DS_FLOW_H
