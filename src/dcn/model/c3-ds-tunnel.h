#ifndef C3_DS_TUNNEL_H
#define C3_DS_TUNNEL_H

#include <cstdint>
#include <map>

#include "c3-tunnel.h"
#include "c3-ds-flow.h"

namespace ns3 {
namespace dcn {

class C3DsTunnel : public C3Tunnel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3DsTunnel (const Ipv4Address &src, const Ipv4Address &dst);

  virtual ~C3DsTunnel ();

  //inherited from C3Tunnel
  virtual void Send (Ptr<Packet> packet, uint8_t protocol);

protected:

  virtual void DoDispose (void);

private:
  /**
   * @brief GetFlow
   * @param fid
   * @return get a flow; create one if not exist
   */
  Ptr<C3Flow> GetFlow (uint32_t fid);

  void Schedule (void);

};

} //namespace dcn
} //namespace ns3

#endif // C3_DS_TUNNEL_H
