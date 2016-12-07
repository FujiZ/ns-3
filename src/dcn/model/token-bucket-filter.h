#ifndef TOKEN_BUCKET_FILTER_H
#define TOKEN_BUCKET_FILTER_H

#include <stdint.h>

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"

namespace ns3 {

class Queue;
class Packet;

/**
 * \ingroup dcn
 *
 * \brief implementation of TBF in ns-2
 * yet another implementation of TBF(Token Bucket filter)
 */
class TokenBucketFilter : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TokenBucketFilter ();
  virtual ~TokenBucketFilter ();

  /**
   * \brief callback to send packets
   */
  typedef Callback<void,Ptr<Packet> > SendTargetCallback;

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
  void GetSendTarget (void);

  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for filter to receive
   */
  void Receive (Ptr<Packet> p);

protected:
  /**
   * \brief This function is called when filter is ready to send a packet
   * \param p the packet to send
   */
  void Send (Ptr<Packet> p);

private:
  /**
   * \brief Update the tokens in TBF
   */
  void UpdateTokens (void);

  /**
   * \brief Schedule the event that send the specific packet
   * \param p the packet to be send
   */
  void ScheduleSend (Ptr<Packet> p);

  /**
   * \brief Timeout function called by timer
   */
  void Timeout (void);

  SendTargetCallback m_sendTarget;
  DataRate m_rate;
  uint64_t m_bucket;
  double m_tokens;
  Time m_lastUpdateTime;
  Ptr<Queue> m_queue;
  Timer m_timer;
};

}


#endif // TOKEN_BUCKET_FILTER_H
