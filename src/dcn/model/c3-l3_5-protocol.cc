#include "c3-l3_5-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C3L3_5Protocol");

NS_OBJECT_ENSURE_REGISTERED (C3L3_5Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t C3L3_5Protocol::PROT_NUMBER = 253;

C3L3_5Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::C3L3_5Protocol")
    .SetParent<IpL3_5Protocol> ()
    .SetGroupName ("DCN")
    .AddConstructor<C3L3_5Protocol> ()
  ;
  return tid;
}

}
