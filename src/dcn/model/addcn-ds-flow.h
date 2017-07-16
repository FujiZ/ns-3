#ifndef ADN_DS_FLOW_H
#define ADN_DS_FLOW_H

#include "addcn-flow.h"

namespace ns3 {
namespace dcn {

class ADNDsFlow : public ADDCNFlow
{
public:

  static TypeId GetTypeId (void);

  ADNDsFlow ();

  virtual ~ADNDsFlow ();

  virtual void UpdateRequestedWeight ();
  virtual bool IsFinished ();
};
}
}
#endif

