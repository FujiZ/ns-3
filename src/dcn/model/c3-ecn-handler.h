#ifndef C3_ECN_HANDLER_H
#define C3_ECN_HANDLER_H

#include <stdint.h>


/**
 * @brief The C3EcnHandler class
 * 1. 统计接收到的ipheader中的ecn信息，作为在c3header中标记ece的依据
 * 2. 统计c3header中的ece信息，作为调整本地发送速率的依据
 * 3. 对于发送的包，根据incoming的ecn信息，间隔标记c3header中的ece
 * 4. 由c3protocol主动调用更新拥塞值
 */
class C3EcnHandler
{
public:
  C3EcnHandler ();

  void Send ();
  void Receive ();
  void UpdateCongestionStatus ();
  void GetAlpha ();
private:
  //ecn stats
  uint32_t m_totalAck;
  uint32_t m_markedAck;
  double m_alpha;
  double m_g;
  ///\todo incoming congestion status
  ///uint32_t m_incomingPackets;
  ///uint32_t m_incomingECNMarkedPackets;
  ///uint32_t m_incomingAlpha;
  ///uint32_t m_incomingG;
};

#endif // C3_ECN_HANDLER_H
