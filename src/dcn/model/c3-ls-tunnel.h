#ifndef C3_LS_TUNNEL_H
#define C3_LS_TUNNEL_H

#include <cstdint>

#include "c3-tunnel.h"

namespace ns3 {
namespace dcn {

class C3LsTunnel : public C3Tunnel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3LsTunnel (uint32_t tenantId, const Ipv4Address &src, const Ipv4Address &dst);

  virtual ~C3LsTunnel ();

  //inherited from C3Tunnel
  virtual void Send (Ptr<Packet> packet, uint8_t protocol);
  virtual void ScheduleFlow (void);

private:
  /**
   * @brief GetFlow
   * @param fid
   * @param protocol
   * @return get a flow; create one if not exist
   */
  Ptr<C3Flow> GetFlow (uint32_t fid, uint8_t protocol);

};

} //namespace dcn
} //namespace ns3

#endif // C3_LS_TUNNEL_H
