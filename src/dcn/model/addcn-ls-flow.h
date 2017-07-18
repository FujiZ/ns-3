#ifndef ADN_LS_FLOW_H
#define ADN_LS_FLOW_H

#include "addcn-flow.h"

namespace ns3 {
namespace dcn {

class ADNLsFlow : public ADDCNFlow
{
public:

  static TypeId GetTypeId (void);

  ADNLsFlow ();

  virtual ~ADNLsFlow ();

  virtual void UpdateRequestedWeight ();
  virtual bool IsFinished ();
};
}
}
#endif

