#ifndef C3_TUNNEL_H
#define C3_TUNNEL_H

#include <map>
#include <stdint.h>

#include "ns3/ptr.h"
#include "ns3/callback.h"

#include "rate-controller.h"

namespace ns3 {
namespace dcn {

/**
 * \ingroup dcn
 * the base class for various type of tunnel (eg: LS, DS)
 */
class C3Tunnel : public RateController
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~C3Tunnel ();
  /**
   * \brief callback to forward packets
   */
  typedef Callback<void, Ptr<Packet> > ForwardTargetCallback;
  /**
   * \brief set forward target
   * \param cb forward target
   */
  void SetForwardTarget (ForwardTargetCallback cb);

  virtual void Send (Ptr<Packet> p) = 0;

protected:
  /**
   * \brief forward a packet to dest
   * \param p packet to be forward
   * usually used for callback
   */
  void Forward (Ptr<Packet> p);

private:
  ForwardTargetCallback m_forwardTarget;  //!< forward target
  //std::map<uint32_t, C3Flow> m_flowMap; //!< fid -> c3Flow
};

} //namespace dcn
} //namespace ns3

#endif // C3_TUNNEL_H
