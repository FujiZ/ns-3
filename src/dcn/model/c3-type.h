#ifndef C3_TYPE_H
#define C3_TYPE_H

#include "stdint.h"

namespace ns3{
namespace dcn{

enum class C3Type : uint8_t
{
  NONE, //!< none type
  DS,   //!< deadline sensitive
  CS,   //!< complete sensitive
  LS,   //!< lantency sensitive
};

}
}

#endif // C3_TYPE_H
