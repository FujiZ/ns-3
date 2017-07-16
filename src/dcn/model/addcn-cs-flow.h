#ifndef ADN_CS_FLOW_H
#define ADN_CS_FLOW_H

#include "addcn-flow.h"

namespace ns3 {
namespace dcn {

class ADNCsFlow : public ADDCNFlow
{
public:

  static TypeId GetTypeId (void);

  ADNCsFlow ();

  virtual ~ADNCsFlow ();

  virtual void UpdateRequestedWeight ();

  double GetWeightMax();
  double GetWeightMin();
private:

  double m_sizeThresh;
  double m_weightMax;
  double m_weightMin;
};
}
}
#endif

