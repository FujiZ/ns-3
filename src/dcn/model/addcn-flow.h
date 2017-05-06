#ifndef ADDCN_FLOW_H
#define ADDCN_FLOW_H

#define DCTCPACK

#include <stdint.h>

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-option.h"
#include "ns3/private/tcp-option-ts.h"
#include "ns3/private/tcp-option-winscale.h"

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

  void SetTenantId(int32_t tenantId);
  /**
   * \brief Set m_segsize param
   * \param size target segment size
   */
  void SetSegmentSize(uint32_t size);

  uint32_t GetSegSize();

  uint32_t GetInitialCwnd ();

  uint32_t GetInitialSSThresh ();
  /**
   * \brief Re-calculate receive window; called on every ACK
   */
  void UpdateReceiveWindow(const TcpHeader &tcpHeader);

  /**
   * \brief Set receive window part of target packet
   * \param packet Target packet
   */
  void SetReceiveWindow(Ptr<Packet> &packet);

  /**
   * \brief At the receiver side, update ecn statistics
   * \param header Ipv4Header where CE mark holds
   */
  void UpdateEcnStatistics(const Ipv4Header &header);
  void UpdateEcnStatistics(const TcpHeader &tcpHeader);

  /**
   * \brief Called every time an ACK was received by the sender
   * \param tcpHeader used for checking sequence&ack numbers
   */
  void UpdateAlpha(const TcpHeader &tcpHeader);

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
  void NotifySend (Ptr<Packet> &packet);
  void NotifyReceive (Ptr<Packet> &packet, const Ipv4Header& header);

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

  double GetScaledWeight (void) const;
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

  SequenceNumber32 GetHighSequenceNumber();
  /**
   * @brief Read and parse the window scale option
   */
  void ProcessOptionWScale(const Ptr<const TcpOption> option);

  void ProcessOptionTimestamp (const Ptr<const TcpOption> option,
                                       const SequenceNumber32 &seq);

  uint32_t BytesInFlight ();

  uint32_t Window ();

  uint32_t GetSsThresh (Ptr<const TcpSocketState> state, uint32_t bytesInFlight);

  void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz, bool isRetransmission);

  void EstimateRtt (const TcpHeader& tcpHeader);

  void ReTxTimeout (); // RTO, Different from TCP, only need to set dupAck = 0, CA_LOSS, CWND & SSThresh

  void NewAck (SequenceNumber32 const& ack, bool resetRTO);

  void DupAck ();

  void ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  uint32_t SafeSubtraction (uint32_t a, uint32_t b);

  void HalveCwnd ();

  void UpdateEcnState (const TcpHeader &tcpHeader);

  void ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  void ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  void ProcessSynRcvd (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  void ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader);

protected:
  virtual void DoDispose (void);

protected:
  int32_t m_tenantId;
  uint32_t m_rWnd;          //!< current receive window
  uint32_t m_flowSize;      //!< the total size of current flow
  uint32_t m_sentSize;      //!< the sent size of current flow
  //int32_t m_bufferedSize;  //!< the size of current buffer
  uint32_t m_segSize;       //!< Setmeng size

  FiveTuple m_tuple; //!< <srcIP, srcPort, dstIP, dstPort, protocol> tuple of current flow


  // parameter about ecn control
  // congestion status
  double m_g;                         //!< parameter g used in alpha updates
  TracedValue<double> m_alpha;        //!< alpha extracted from ECN feedback, indicating flow congestion status
  Ptr<C3EcnRecorder> m_ecnRecorder;   //!< ecn recorder
  double m_ackedBytesEcn;
  double m_ackedBytesTotal;
  bool   m_ecnEnabled;                //!< whether upper layer supports ECN
  bool   m_ceReceived;

  // flow weight parameter
  TracedValue<double> m_scale;   //!< scale updated by corresponding slice
  TracedValue<double> m_weight;  //!< real flow weight
  //DataRate m_rate;  //!< current flow rate

  bool m_winScalingEnabled;
  bool m_timestampEnabled;

private:
  TracedValue<double> m_weightScaled;   //!< flow scaled weight
  SequenceNumber32 m_seqNumber;         //!< highest sequence number sent
  SequenceNumber32 m_updateRwndSeq;         //!< last sequence number upon which window was cut
  SequenceNumber32 m_updateAlphaSeq;        //!< last sequence number upon which alpha was updated
  SequenceNumber32 m_dctcpMaxSeq;           //!< 
  SequenceNumber32 m_highRxAckMark;
  uint8_t m_sndWindShift;                  //!< Window shift to apply to incoming segments

  SequenceNumber32 m_recover;
  SequenceNumber32 m_ecnEchoSeq;
  SequenceNumber32 m_SND_UNA;           //!<
  SequenceNumber32 m_SND_NXT;           //!<
  //Ptr<TokenBucketFilter> m_tbf; //!< tbf to control rate
  ForwardTargetCallback m_forwardTarget;    //!< callback to forward packet

private:
  /*
  uint32_t              m_retxThresh;
  bool                  m_limitedTx;
  uint32_t              m_retransOut;
  */
  
  EventId               m_retxEvent;
  Time                  m_rto;
  Time                  m_minRto;
  Time                  m_lastRtt;
  Time                  m_clockGranularity;

  TcpSocket::TcpStates_t           m_state;
  uint8_t               m_ecnState;
  uint32_t              m_dupAckCount;
  uint32_t              m_retransOut; 
  uint32_t              m_bytesAckedNotProcessed;
  bool                  m_isFirstPartialAck;
  bool                  m_ecnTransition;
  Ptr<TcpSocketState>   m_tcb;
  Ptr<RttEstimator>     m_rtt;
  RttHistory_t          m_history;  
  //Ptr<TcpCongestionOps> m_congestionControl;
  //bool m_isFirstPartialAck;
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
