#ifndef C3_DIVISION_H
#define C3_DIVISION_H

#include <map>

#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/callback.h"
#include "ns3/ipv4-header.h"

#include "rate-controller.h"

namespace ns3 {

class Packet;
class Ipv4Route;
class Ipv4Interface;

namespace dcn {

class C3Tunnel;

/**
 * \ingroup dcn
 *
 * \brief c3 division implementation
 *
 */
class C3Division : public RateController
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  C3Division (Ipv4Address src, Ipv4Address dst);

  virtual ~C3Division ();

  /**
   * \brief set the route of connection
   * \param route new route info
   */
  void SetRoute (Ptr<Ipv4Route> route);

  /**
   * \brief callback to forward packets to dest
   */
  typedef Callback<void, Ptr<Packet>,
  Ipv4Address, Ipv4Address, Ptr<Ipv4Route> > ForwardTargetCallback;

  /**
   * \brief set forward target
   */
  void SetForwardTargetCallback(ForwardTargetCallback cb);

  /**
   * \todo callback to send packets back to src
   * backwardTargetCallback
   */
protected:
  virtual void DoDispose (void);

  /**
   * \brief forward a packet to dest
   * usually used for callback
   */
  void Forward (Ptr<Packet> p);

private:
  Ipv4Address m_source;   //!< source address of division
  Ipv4Address m_destination;  //!< dst address of division
  Ptr<Ipv4Route> m_route; //!< route of connection
  ForwardTargetCallback m_forwardTargetCallback;  //!< forward target
  //std::map<C3Tag, C3Tunnel> tunnels;
  //c3tag, c3dstag, c3cstag etc.
  //std::map<fid_t, C3Tunnel> flows;
};

} //namespace dcn
} //namespace ns3

#endif // C3_DIVISION_H
