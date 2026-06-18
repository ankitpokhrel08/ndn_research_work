#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "blackhole-producer.hpp"
#include <fstream>
#include <sstream>

using namespace ns3;

int main(int argc, char* argv[])
{
  double   simTime     = 600.0;
  double   attackStart = 300.0;
  double   attackRate   = 100.0;  // total attacker interest rate (interests/s)
  uint32_t rngRun      = 1;      // replication seed

  CommandLine cmd;
  cmd.AddValue("simTime",     "Simulation time in seconds", simTime);
  cmd.AddValue("attackStart", "Attack start time",          attackStart);
  cmd.AddValue("attackRate",  "Per-attacker interest rate (interests/s)", attackRate);
  cmd.AddValue("rngRun",      "RNG run number for replication",          rngRun);
  cmd.Parse(argc, argv);

  ns3::RngSeedManager::SetSeed(1);
  ns3::RngSeedManager::SetRun(rngRun);

  system("mkdir -p results");

  std::ostringstream tag;
  tag << "dumbbell-ifa-r" << (int)attackRate << "-run" << rngRun;
  std::string base = "results/" + tag.str();

  NodeContainer consumers, producers;
  consumers.Create(3);
  producers.Create(2);

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
  Ptr<Node> bn = bottleneckContainer.Get(0);
  Ptr<Node> r3 = rightRouters.Get(0);
  Ptr<Node> r4 = rightRouters.Get(1);
  Ptr<Node> p1 = producers.Get(0);
  Ptr<Node> p2 = producers.Get(1);

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

  // Non-responding producer for /evil attack prefix
  ns3::ndn::AppHelper blackhole("ns3::ndn::BlackholeProducer");
  blackhole.SetAttribute("Prefix", StringValue("/evil"));
  blackhole.Install(p1);
  blackhole.Install(p2);
  grHelper.AddOrigins("/evil", p1);
  grHelper.AddOrigins("/evil", p2);

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

  // IFA: c1 floods fake interests across the bottleneck
  // This is the key stress point for dumbbell — all fake traffic
  // must traverse r2->bn->r3 competing with legitimate traffic

  ns3::ndn::AppHelper atk1("ns3::ndn::ConsumerCbr");
  atk1.SetAttribute("Prefix", StringValue("/evil/fake/video"));
  atk1.SetAttribute("Frequency", StringValue(std::to_string(0.40 * attackRate)));
  atk1.SetAttribute("LifeTime", StringValue("2s"));
  auto a1 = atk1.Install(c1);
  a1.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk2("ns3::ndn::ConsumerCbr");
  atk2.SetAttribute("Prefix", StringValue("/evil/random-attack"));
  atk2.SetAttribute("Frequency", StringValue(std::to_string(0.30 * attackRate)));
  atk2.SetAttribute("LifeTime", StringValue("1500ms"));
  auto a2 = atk2.Install(c1);
  a2.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk3("ns3::ndn::ConsumerCbr");
  atk3.SetAttribute("Prefix", StringValue("/evil/fake/image"));
  atk3.SetAttribute("Frequency", StringValue(std::to_string(0.20 * attackRate)));
  atk3.SetAttribute("LifeTime", StringValue("1s"));
  auto a3 = atk3.Install(c1);
  a3.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk4("ns3::ndn::ConsumerCbr");
  atk4.SetAttribute("Prefix", StringValue("/evil/random-attack-2"));
  atk4.SetAttribute("Frequency", StringValue(std::to_string(0.10 * attackRate)));
  atk4.SetAttribute("LifeTime", StringValue("4s"));
  auto a4 = atk4.Install(c1);
  a4.Start(Seconds(attackStart));

  ns3::ndn::L3RateTracer::InstallAll((base + "-rate-trace.txt").c_str(), Seconds(1.0));
  ns3::ndn::CsTracer::InstallAll((base + "-cs-trace.txt").c_str(), Seconds(1.0));
  ns3::ndn::AppDelayTracer::InstallAll((base + "-app-delays.txt").c_str());

  std::ofstream gt(base + "-ground-truth.csv");
  gt << "time,label\n";
  for (double t = 0; t < simTime; t += 1.0)
    gt << t << "," << (t < attackStart ? "normal" : "ifa") << "\n";
  gt.close();

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}