#ifndef C3_FLOW_H
#define C3_FLOW_H

#include <stdint.h>

#include "ns3/object.h"
#include "ns3/packet.h"

#include "token-bucket-filter.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 flow
 * the base class for various type of flow (eg: LS, DS)
 */
class C3Flow : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Flow ();

  virtual ~C3Flow ();

  /**
   * \brief callback to forward packets
   */
  typedef Callback<void, Ptr<Packet>, uint8_t> ForwardTargetCallback;

  /**
   * \brief set forward target
   * \param cb forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);

  /**
   * @brief Set Protocol number
   * @param protocol l4 protocol number
   */
  void SetProtocol (uint8_t protocol);

  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for controller to receive
   * make sure that the packet contain c3tag before pass in it
   * the packet size should be marked in c3l3.5p
   */
  virtual void Send (Ptr<Packet> packet);

  /**
   * @brief Update tunnel info (weight)
   * called by upper tunnel
   * the update of weight and rate is seperate
   */
  virtual void UpdateInfo (void) = 0;

  /**
   * @brief GetWeight
   * @return flow weight
   */
  double GetWeight (void) const;

  /**
   * @brief SetRate
   * @param rate
   */
  void SetRate (DataRate rate);

protected:

  virtual void DoDispose (void);

  /**
   * @brief Forward callback to forward packets
   * @param packet the packet tobe sent out
   * usually used as callback
   */
  virtual void Forward (Ptr<Packet> packet);

  /**
   * @brief Drop callback to drop packet
   * @param packet the packet to drop
   * used as callback
   */
  virtual void Drop (Ptr<const Packet> packet);

protected:

  int32_t m_flowSize;    //!< the total size of current flow
  int32_t m_sentBytes;    //!< the sent size of current flow
  int32_t m_bufferedBytes;  //!< the size of current buffer
  double m_weight;  //!< weight

private:

  /**
   * @brief GetPacketSize
   * @param packet
   * @param protocol the protocol number of packet
   * @return packet size in bytes
   * Get the data field size of a packet
   */
  uint32_t GetPacketSize (Ptr<const Packet> packet) const;

  uint8_t m_protocol;    //!< the protocol number of current flow
  Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  ForwardTargetCallback m_forwardTarget;    //!< callback to forward packet
};

} //namespace dcn
} //namespace ns3

#endif // C3_FLOW_H
