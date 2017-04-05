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
  virtual void SlowDown (void);
  virtual void OpenCwnd (uint32_t segmentAcked);

  double GetWeightC (void);

  double m_weightMax;
  double m_weightMin;

  TracedValue<uint64_t> m_sentBytes;        //!< bytes already sent

};

} // namespace ns3

#endif // L2DCT_SOCKET_H
