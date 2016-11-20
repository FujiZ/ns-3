#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcnTest");

int
main (int argc, char *argv[])
{
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
}

