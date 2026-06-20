#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <fstream>

using namespace ns3;

int main(int argc, char* argv[])
{
  double simTime = 600.0;

  CommandLine cmd;
  cmd.AddValue("simTime", "Simulation time in seconds", simTime);
  cmd.Parse(argc, argv);

  system("mkdir -p results");

  // Dumbbell has a dedicated bottleneck node — 3 consumers, 4 routers+bottleneck, 2 producers
  NodeContainer consumers, producers;
  consumers.Create(3);   // c1, c2, c3
  producers.Create(2);   // p1, p2

  // r1, r2 on consumer side; bottleneck; r3, r4 on producer side
  NodeContainer leftRouters, rightRouters, bottleneckContainer;
  leftRouters.Create(2);
  bottleneckContainer.Create(1);
  rightRouters.Create(2);

  NodeContainer allNodes;
  allNodes.Add(consumers);
  allNodes.Add(leftRouters);
  allNodes.Add(bottleneckContainer);
  allNodes.Add(rightRouters);
  allNodes.Add(producers);

  Ptr<Node> c1 = consumers.Get(0);
  Ptr<Node> c2 = consumers.Get(1);
  Ptr<Node> c3 = consumers.Get(2);
  Ptr<Node> r1 = leftRouters.Get(0);
  Ptr<Node> r2 = leftRouters.Get(1);
  Ptr<Node> bn = bottleneckContainer.Get(0); // bottleneck
  Ptr<Node> r3 = rightRouters.Get(0);
  Ptr<Node> r4 = rightRouters.Get(1);
  Ptr<Node> p1 = producers.Get(0);
  Ptr<Node> p2 = producers.Get(1);

  // Standard links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));

  p2p.Install(c1, r1);
  p2p.Install(c2, r1);
  p2p.Install(c3, r1);
  p2p.Install(r1, r2);
  p2p.Install(r3, r4);
  p2p.Install(r3, p1);
  p2p.Install(r4, p2);

  // Bottleneck links — bw=10 from README, same rate but marked separately
  // for clarity and so you can easily restrict it further if needed
  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  bottleneckLink.SetChannelAttribute("Delay", StringValue("10ms"));

  bottleneckLink.Install(r2, bn);
  bottleneckLink.Install(bn, r3);

  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(100);
  ndnHelper.Install(allNodes);

  ns3::ndn::GlobalRoutingHelper grHelper;
  grHelper.Install(allNodes);

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("Prefix", StringValue("/ndn"));
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(p1);
  producerHelper.Install(p2);

  grHelper.AddOrigins("/ndn", p1);
  grHelper.AddOrigins("/ndn", p2);
  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("Prefix", StringValue("/ndn"));
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.SetAttribute("NumberOfContents", StringValue("100"));
  consumerHelper.SetAttribute("q", StringValue("0.0"));
  consumerHelper.SetAttribute("s", StringValue("0.7"));
  consumerHelper.Install(c1);
  consumerHelper.Install(c2);
  consumerHelper.Install(c3);

  ns3::ndn::L3RateTracer::InstallAll("results/dumbbell-normal-rate-trace.txt", Seconds(1.0));
  ns3::ndn::CsTracer::InstallAll("results/dumbbell-normal-cs-trace.txt", Seconds(1.0));
  ns3::ndn::AppDelayTracer::InstallAll("results/dumbbell-normal-app-delays.txt");

  std::ofstream gt("results/dumbbell-normal-ground-truth.csv");
  gt << "time,label\n";
  for (double t = 0; t < simTime; t += 1.0)
    gt << t << ",normal\n";
  gt.close();

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}