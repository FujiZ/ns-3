#ifndef C3_DSTUNNEL_H
#define C3_DSTUNNEL_H

#include <stdint.h>
#include <map>
#include <vector>

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
  C3DsTunnel ();
  virtual ~C3DsTunnel ();

  //inherited from RateController
  virtual DataRate UpdateRateRequest (void);
  virtual void SetRateResponse (const DataRate &rate);
  //inherited from C3Tunnel
  virtual void Send (Ptr<Packet> p);

protected:

private:
  void Schedule (void);

private:
  std::map<uint32_t, Ptr<C3DsFlow> > flowMap;
  std::vector<Ptr<C3DsFlow> > flowVec;
};

} //namespace dcn
} //namespace ns3

#endif // C3_DSTUNNEL_H
