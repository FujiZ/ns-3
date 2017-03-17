#ifndef DCTCP_SOCKET_H
#define DCTCP_SOCKET_H

#include "tcp-socket-base.h"

namespace ns3 {

class DctcpSocket : public TcpSocketBase
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
  DctcpSocket(void);

  /**
   * Clone a TCP socket, for use upon receiving a connection request in LISTEN state
   *
   * \param sock the original Tcp Socket
   */
  DctcpSocket (const DctcpSocket& sock);

protected:

  // inherited from TcpSocketBase
  virtual void DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                            const Address &toAddress);

  virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);

  virtual void SendAckPacket (void);

  virtual void EstimateRtt (const TcpHeader& tcpHeader);

  virtual void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                 bool isRetransmission);

  virtual void Retransmit (void);

  virtual Ptr<TcpSocketBase> Fork (void);

  void UpdateAlpha (const TcpHeader &tcpHeader);

  // DCTCP related params
  double m_g;   //!< dctcp g param
  TracedValue<double> m_alpha;   //!< dctcp alpha param
  uint32_t m_ackedBytesEcn; //!< acked bytes with ecn
  uint32_t m_ackedBytesTotal;   //!< acked bytes total
  SequenceNumber32 m_alphaUpdateSeq;
  SequenceNumber32 m_dctcpMaxSeq;
  bool m_ceTransition; //!< ce state machine to support delayed ACK
  bool m_ecnTransition; //!< flag for determing if we should send ECE packet
};

} // namespace ns3

#endif // DCTCP_SOCKET_H
