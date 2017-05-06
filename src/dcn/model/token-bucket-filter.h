#ifndef TOKEN_BUCKET_FILTER_H
#define TOKEN_BUCKET_FILTER_H

#include <stdint.h>

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/packet.h"

#include "connector.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief implement the ns-2 TBF in ns-3
 * yet another implementation of TBF(Token Bucket filter)
 */
class TokenBucketFilter : public Connector
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TokenBucketFilter ();
  virtual ~TokenBucketFilter ();

  //inherited from Connector
  virtual void Send (Ptr<Packet> p);

  /**
   * @brief GetRate
   * @return rate for tbf
   */
  DataRate GetRate (void) const;
  /**
   * @brief SetRate
   * @param rate the rate to set
   * set the rate of tbf
   */
  void SetRate (DataRate rate);

  /**
   * @brief GetQueueLimit
   * @return max queue length
   */
  uint32_t GetQueueLimit (void) const;

  /**
   * @brief SetQueueLimit
   * @param limit
   */
  void SetQueueLimit (uint32_t limit);

protected:

  virtual void DoDispose (void);

private:

  /**
   * \brief called when packet drop
   * \param item droped item
   * this is mainly used for callback
   */
  void Drop (Ptr<QueueItem> item);
  /**
   * \brief Update the tokens in TBF
   */
  void UpdateTokens (void);
  /**
   * \brief get the delay time to send the packet
   * \param p the packet to be send
   * \return the time delay
   */
  Time GetSendDelay (Ptr<const Packet> p) const;
  /**
   * \brief Sending the first Packet in the queue to the sendTarget
   */
  void Transmit (void);

private:
  DataRate m_rate;
  uint64_t m_bucket;
  double m_tokens;
  bool m_init;
  Time m_lastUpdateTime;
  Ptr<Queue> m_queue;
  Timer m_timer;
};

} //namespace dcn
} //namespace ns3

#endif // TOKEN_BUCKET_FILTER_H
