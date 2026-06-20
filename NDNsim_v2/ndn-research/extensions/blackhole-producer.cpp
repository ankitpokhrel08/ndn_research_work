#include "blackhole-producer.hpp"

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.BlackholeProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(BlackholeProducer);

TypeId
BlackholeProducer::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::BlackholeProducer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<BlackholeProducer>()
      .AddAttribute("Prefix", "Prefix to register and then ignore",
                    StringValue("/"),
                    MakeNameAccessor(&BlackholeProducer::m_prefix),
                    MakeNameChecker());
  return tid;
}

BlackholeProducer::BlackholeProducer() {}

void
BlackholeProducer::StartApplication()
{
  App::StartApplication();
  // Register the prefix on our app face so NFD delivers these interests
  // here instead of NACKing them. m_face is created by App::StartApplication().
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
BlackholeProducer::StopApplication()
{
  App::StopApplication();
}

void
BlackholeProducer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  // Deliberately no Data sent. The interest sits in the PIT and times out.
}

} // namespace ndn
} // namespace ns3