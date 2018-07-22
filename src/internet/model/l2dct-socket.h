#ifndef L2DCT_SOCKET_H
#define L2DCT_SOCKET_H

#include "dctcp-socket.h"

namespace ns3 {

class L2dctSocket : public DctcpSocket
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the instance TypeId
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId () const;

  /**
   * Create an unbound TCP socket
   */
  L2dctSocket (void);

  /**
   * Clone a TCP socket, for use upon receiving a connection request in LISTEN state
   *
   * \param sock the original Tcp Socket
   */
  L2dctSocket (const L2dctSocket& sock);

protected:

  // update sentBytes in UpdateRttHistory
  virtual void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                 bool isRetransmission);
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual uint32_t GetSsThresh (void);
  virtual void IncreaseWindow (uint32_t segmentAcked);

  uint32_t SlowStart (uint32_t segmentsAcked);
  void CongestionAvoidance (uint32_t segmentsAcked);
  double GetWeightC (void) const;


  double m_weightMax;
  double m_weightMin;
  TracedValue<uint64_t> m_sentBytes;        //!< bytes already sent

};

} // namespace ns3

#endif // L2DCT_SOCKET_H
