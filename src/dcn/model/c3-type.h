#ifndef C3_TYPE_H
#define C3_TYPE_H

namespace ns3{
namespace dcn{

enum class C3Type
{
  NONE, //!< none type
  DS,   //!< deadline sensitive
  CS,   //!< complete sensitive
  LS,   //!< lantency sensitive
};

}
}

#endif // C3_TYPE_H
