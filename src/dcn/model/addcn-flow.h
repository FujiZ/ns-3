#ifndef ADDCN_FLOW_H
#define ADDCN_FLOW_H

#include <stdint.h>

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-route.h"

#include "token-bucket-filter.h"
#include "c3-ecn-recorder.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 *
 * \brief c3 flow
 * the base class for various type of flow (eg: LS, DS)
 */
class ADDCNFlow : public Object
{
public:
/// Structure to classify a packet
  struct FiveTuple
  {
    Ipv4Address sourceAddress;      //!< Source address
    Ipv4Address destinationAddress; //!< Destination address
    uint8_t protocol;               //!< Protocol
    uint16_t sourcePort;            //!< Source port
    uint16_t destinationPort;       //!< Destination port
  };
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ADDCNFlow ();

  virtual ~ADDCNFlow ();

  /**
   * \brief Initialize parameters
   */
  void Initialize();

  /**
   * \brief Set m_segsize param
   * \param size target segment size
   */
  void SetSegmentSize(int32_t size);

  /**
   * \brief Re-calculate receive window; called on every ACK
   */
  void UpdateReceiveWindow();

  /**
   * \brief Set receive window part of target packet
   * \param packet Target packet
   */
  void SetReceiveWindow(Ptr<Packet> &packet);

  /**
   * \brief callback to forward packets
   */
  typedef Callback<void, Ptr<Packet>, Ipv4Address, Ipv4Address, uint8_t, Ptr<Ipv4Route> > ForwardTargetCallback;

  /**
   * \brief set forward target
   * \param cb forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);

  /**
   * \brief set Ipv4 FiveTuple <srcIP, dstIP, srcPort, dstPort, protocol>
   * \param tuple target tuple
   */
  void SetFiveTuple (ADDCNFlow::FiveTuple tuple);

  /**
   * \brief This function is called by sender end when sending packets
   * \param p the packet for controller to receive
   * make sure that the packet contain c3tag before pass in it
   * the packet size should be marked in c3l3.5p
   */
  void Send (Ptr<Packet> packet, Ptr<Ipv4Route> route);

  /**
   * @brief Update tunnel info (weight)
   * called by upper division
   * @todo maybe the update of weight and rate should seperate
   */
  //virtual void UpdateInfo (void) = 0;

  /**
   * @brief GetWeight
   * @return flow weight
   */
  double GetWeight (void) const;

  /**
   * @brief Update scale factor, called by corresponding slice
   * @param s
   */
  void UpdateScale(const double s);

  /**
   * @brief UpdateAlpha
   * update alpha value
   */
  void UpdateAlpha (void);

protected:
  virtual void DoDispose (void);

protected:
  int32_t m_rwnd;          //!< current receive window
  int32_t m_flowSize;      //!< the total size of current flow
  int32_t m_sentSize;      //!< the sent size of current flow
  //int32_t m_bufferedSize;  //!< the size of current buffer
  int32_t m_segSize;       //!< Setmeng size

  FiveTuple m_tuple; //!< <srcIP, srcPort, dstIP, dstPort, protocol> tuple of current flow


  // parameter about ecn control
  // congestion status
  double m_g;                         //!< parameter g used in alpha updates
  TracedValue<double> m_alpha;        //!< alpha extracted from ECN feedback, indicating flow congestion status
  Ptr<C3EcnRecorder> m_ecnRecorder;   //!< ecn recorder

  // flow weight parameter
  TracedValue<double> m_scale;   //!< scale updated by corresponding slice
  TracedValue<double> m_weight;  //!< real flow weight
  //DataRate m_rate;  //!< current flow rate

private:
  TracedValue<double> m_weightScaled;   //!< flow scaled weight

  //Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  ForwardTargetCallback m_forwardTarget;    //!< callback to forward packet
};

/**
 * \brief Less than operator.
 *
 * \param t1 the first operand
 * \param t2 the first operand
 * \returns true if the operands are equal
 */
bool operator < (const ADDCNFlow::FiveTuple &t1, const ADDCNFlow::FiveTuple &t2);

/**
 * \brief Equal to operator.
 *
 * \param t1 the first operand
 * \param t2 the first operand
 * \returns true if the operands are equal
 */
bool operator == (const ADDCNFlow::FiveTuple &t1, const ADDCNFlow::FiveTuple &t2);

} //namespace dcn
} //namespace ns3

#endif // ADDCN_FLOW_H
