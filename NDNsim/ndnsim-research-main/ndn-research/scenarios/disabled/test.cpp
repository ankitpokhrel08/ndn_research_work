#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
    NodeContainer nodes;
    nodes.Create(2);

    ndn::StackHelper ndnHelper;
    ndnHelper.InstallAll();

    Simulator::Stop(Seconds(1.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
