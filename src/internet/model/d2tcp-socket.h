#ifndef D2TCP_SOCKET_H
#define D2TCP_SOCKET_H

#include "dctcp-socket.h"

namespace ns3 {

class D2tcpSocket : public DctcpSocket
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
  D2tcpSocket (void);

  /**
   * Clone a TCP socket, for use upon receiving a connection request in LISTEN state
   *
   * \param sock the original Tcp Socket
   */
  D2tcpSocket (const D2tcpSocket& sock);

  virtual int Connect (const Address &address);      // Setup endpoint and call ProcessAction() to connect

protected:

  // virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);

  // update sentBytes in UpdateRttHistory
  virtual void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                 bool isRetransmission);
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual uint32_t GetSsThresh (void);

  /**
   * @brief Check Deadline before sending a packet;
   * @return true if not exceeded
  bool CheckDeadline (void) const;
   */

  // D2TCP related params
  Time                  m_deadline;         //!< deadline of current flow
  Time                  m_finishTime;       //!< actual finish time in real
  uint64_t              m_totalBytes;       //!< total bytes to send
  TracedValue<uint64_t> m_sentBytes;        //!< bytes already sent
};

/**
 * TracedValue Callback signature for Uint64
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
typedef void (* Uint64TracedValueCallback)(const uint64_t oldValue, const uint64_t newValue);
 */

} // namespace ns3

#endif // D2TCP_SOCKET_H
