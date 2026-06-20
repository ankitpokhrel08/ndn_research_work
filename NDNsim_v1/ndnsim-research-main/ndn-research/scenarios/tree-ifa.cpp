#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "blackhole-producer.hpp"
#include <fstream>

using namespace ns3;

int main(int argc, char* argv[])
{
  double simTime = 600.0;
  double attackStart = 300.0; // 5 min baseline, 5 min attack = balanced classes

  CommandLine cmd;
  cmd.AddValue("simTime",     "Simulation time in seconds", simTime);
  cmd.AddValue("attackStart", "Attack start time",          attackStart);
  cmd.Parse(argc, argv);

  system("mkdir -p results");

  NodeContainer producers, routers, consumers;
  producers.Create(2);
  routers.Create(4);
  consumers.Create(6);

  NodeContainer allNodes;
  allNodes.Add(producers);
  allNodes.Add(routers);
  allNodes.Add(consumers);

  Ptr<Node> p1 = producers.Get(0);
  Ptr<Node> p2 = producers.Get(1);
  Ptr<Node> r1 = routers.Get(0);
  Ptr<Node> r2 = routers.Get(1);
  Ptr<Node> r3 = routers.Get(2);
  Ptr<Node> r4 = routers.Get(3);
  Ptr<Node> c1 = consumers.Get(0);
  Ptr<Node> c2 = consumers.Get(1);
  Ptr<Node> c3 = consumers.Get(2);
  Ptr<Node> c4 = consumers.Get(3);
  Ptr<Node> c5 = consumers.Get(4);
  Ptr<Node> c6 = consumers.Get(5);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));

  p2p.Install(p1, r1);
  p2p.Install(p2, r1);
  p2p.Install(r1, r2);
  p2p.Install(r1, r3);
  p2p.Install(r2, r4);
  p2p.Install(r3, r4);
  p2p.Install(r2, c1);
  p2p.Install(r2, c2);
  p2p.Install(r3, c3);
  p2p.Install(r3, c4);
  p2p.Install(r4, c5);
  p2p.Install(r4, c6);

  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true); // deterministic routing, no flooding/loops
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

  // Non-responding producer for /evil, co-located on the real producers.
  // Attack interests route here through the core and expire -> PIT pressure + timeouts.
  ns3::ndn::AppHelper blackhole("ns3::ndn::BlackholeProducer");
  blackhole.SetAttribute("Prefix", StringValue("/evil"));
  blackhole.Install(p1);
  blackhole.Install(p2);
  grHelper.AddOrigins("/evil", p1);
  grHelper.AddOrigins("/evil", p2);
  // NOTE: no producerHelper for /evil — only the blackhole serves it

  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();

  // Legitimate consumers — run entire simulation
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("Prefix", StringValue("/ndn"));
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.SetAttribute("NumberOfContents", StringValue("100"));
  consumerHelper.SetAttribute("q", StringValue("0.0"));
  consumerHelper.SetAttribute("s", StringValue("0.7"));
  consumerHelper.Install(c1);
  consumerHelper.Install(c2);
  consumerHelper.Install(c3);
  consumerHelper.Install(c4);
  consumerHelper.Install(c5);
  consumerHelper.Install(c6);

  // IFA: c1 becomes attacker at attackStart
  // 4 fake prefixes with different lifetimes matching README spec
  // frequencies scaled proportionally (40:30:20:10)

  ns3::ndn::AppHelper atk1("ns3::ndn::ConsumerCbr");
  atk1.SetAttribute("Prefix", StringValue("/evil/fake/video"));
  atk1.SetAttribute("Frequency", StringValue("40"));
  atk1.SetAttribute("LifeTime", StringValue("2s"));
  auto a1 = atk1.Install(c1);
  a1.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk2("ns3::ndn::ConsumerCbr");
  atk2.SetAttribute("Prefix", StringValue("/evil/random-attack"));
  atk2.SetAttribute("Frequency", StringValue("30"));
  atk2.SetAttribute("LifeTime", StringValue("1500ms"));
  auto a2 = atk2.Install(c1);
  a2.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk3("ns3::ndn::ConsumerCbr");
  atk3.SetAttribute("Prefix", StringValue("/evil/fake/image"));
  atk3.SetAttribute("Frequency", StringValue("20"));
  atk3.SetAttribute("LifeTime", StringValue("1s"));
  auto a3 = atk3.Install(c1);
  a3.Start(Seconds(attackStart));

  ns3::ndn::AppHelper atk4("ns3::ndn::ConsumerCbr");
  atk4.SetAttribute("Prefix", StringValue("/evil/random-attack-2"));
  atk4.SetAttribute("Frequency", StringValue("10"));
  atk4.SetAttribute("LifeTime", StringValue("4s"));
  auto a4 = atk4.Install(c1);
  a4.Start(Seconds(attackStart));

  ns3::ndn::L3RateTracer::InstallAll("results/tree-ifa-rate-trace.txt", Seconds(1.0));
  ns3::ndn::CsTracer::InstallAll("results/tree-ifa-cs-trace.txt", Seconds(1.0));
  ns3::ndn::AppDelayTracer::InstallAll("results/tree-ifa-app-delays.txt");

  std::ofstream gt("results/tree-ifa-ground-truth.csv");
  gt << "time,label\n";
  for (double t = 0; t < simTime; t += 1.0)
    gt << t << "," << (t < attackStart ? "normal" : "ifa") << "\n";
  gt.close();

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}